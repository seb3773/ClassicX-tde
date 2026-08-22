#include "classicx_applet.h"
#include <tqlayout.h>
#include <tdelocale.h>
#include <tdeglobal.h>
#include <tqtooltip.h>
#include <dcopobject.h>
#include <tqtimer.h>
#include <kprocess.h>
#include <kstandarddirs.h>
#include <tdeapplication.h>
#include <tdeconfig.h>
#include <dcopclient.h>
#include <kiconloader.h>
#include "embedded_icons.h"
#include "menumanager.h"
#include "k_mnu_stub.h"
#include "global.h"
#include "kickertip.h"
#include "classicxSettings.h"

static MenuManager* findKickerMenuManager()
{
    DCOPObject *obj = DCOPObject::find("MenuManager");
    if (obj) {
        return dynamic_cast<MenuManager*>(obj);
    }
    return 0;
}

extern "C"
{
    KDE_EXPORT KPanelApplet* init(TQWidget *parent, const TQString& configFile)
    {
        TDEGlobal::locale()->insertCatalogue("classicxapplet");
        TDEGlobal::locale()->insertCatalogue("kicker");
        return new ClassicXApplet(configFile, KPanelApplet::Normal,
                               KPanelApplet::Preferences,
                               parent, "ClassicXApplet");
    }
}

ClassicXButton::ClassicXButton(TQWidget *parent)
    : PanelPopupButton(parent, "ClassicXButton")
{
    TQToolTip::add(this, i18n("Applications, tasks and desktop sessions"));
    setTitle(i18n("Classic-X Menu"));
    loadIcons();
    configure();
}

void ClassicXButton::configure()
{
    PanelButton::configure();
}

ClassicXButton::~ClassicXButton()
{
    MenuManager *mgr = findKickerMenuManager();
    if (mgr) {
        mgr->unregisterKButton(this);
    }
}

void ClassicXButton::showMenu()
{
    ClassicXApplet *applet = dynamic_cast<ClassicXApplet*>(parent());
    if (applet) {
        applet->showMenu();
    } else {
        realShowMenu();
    }
}

void ClassicXButton::mousePressEvent(TQMouseEvent *e)
{
    if (e->button() == Qt::LeftButton) {
        e->accept();
        showMenu();
        return;
    } else if (e->button() == Qt::RightButton) {
        e->accept();

        TDEConfig kickerCfg("kickerrc");
        kickerCfg.reparseConfiguration();
        kickerCfg.setGroup("General");
        bool isLocked = kickerCfg.readBoolEntry("Locked", false);

        TQGuardedPtr<ClassicXButton> gThis = this;
        TQPopupMenu popup(0);
        popup.insertItem(EmbeddedIcons::getSmallIconSet("menu-settings"), i18n("Configure Classic-X Menu..."), this, TQT_SLOT(slotConfigure()));
        popup.insertItem(EmbeddedIcons::getSmallIconSet("kmenuedit"), i18n("Edit Menu"), this, TQT_SLOT(slotEditMenu()));
        popup.insertSeparator();
        popup.insertItem(SmallIconSet("configure"), i18n("&Configure Panel..."), this, TQT_SLOT(slotConfigurePanel()));
        popup.insertItem(isLocked ? SmallIconSet("unlock") : SmallIconSet("system-lock-screen"),
                          isLocked ? i18n("Un&lock Panels") : i18n("&Lock Panels"),
                          kapp, TQT_SLOT(toggleLock()));

        popup.exec(e->globalPos());
        if (!gThis) return;
        return;
    }
    PanelPopupButton::mousePressEvent(e);
}

void ClassicXButton::mouseReleaseEvent(TQMouseEvent *e)
{
    if (e->button() == Qt::LeftButton) {
        e->accept();
        return;
    }
    PanelPopupButton::mouseReleaseEvent(e);
}

void ClassicXButton::slotConfigure()
{
    ClassicXApplet *applet = dynamic_cast<ClassicXApplet*>(parent());
    if (applet) {
        applet->preferences();
    }
}

void ClassicXButton::slotEditMenu()
{
    TDEProcess *proc = new TDEProcess;
    connect(proc, TQT_SIGNAL(processExited(TDEProcess*)), proc, TQT_SLOT(deleteLater()));
    *proc << TDEStandardDirs::findExe(TQString::fromLatin1("kmenuedit"));
    if (!proc->start(TDEProcess::DontCare)) {
        delete proc;
    }
}

void ClassicXButton::slotConfigurePanel()
{
    TQByteArray data;
    TQDataStream stream(data, IO_WriteOnly);
    stream << TQString("") << TQString("") << (int)-1;
    if (kapp && kapp->dcopClient() && !kapp->dcopClient()->send("kicker", "kicker", "showConfig(TQString,TQString,int)", data)) {
        TDEProcess *proc = new TDEProcess;
        connect(proc, TQT_SIGNAL(processExited(TDEProcess*)), proc, TQT_SLOT(deleteLater()));
        *proc << TDEStandardDirs::findExe(TQString::fromLatin1("kcmshell")) << "tde-panel";
        if (!proc->start(TDEProcess::DontCare)) {
            delete proc;
        }
    }
}

ClassicXApplet::ClassicXApplet(const TQString& configFile, Type type, int actions, TQWidget *parent, const char *name)
    : KPanelApplet(configFile, type, actions, parent, name)
    , m_panelHeight(0)
    , m_button(0)
    , m_menu(0)
    , m_menuManagerPrevious(0)
{
    TQHBoxLayout* layout = new TQHBoxLayout(this);
    layout->setMargin(0);
    
    m_button = new ClassicXButton(this);
    m_button->setDrawArrow(true);
    m_button->setPopupDirection(popupDirection());
    m_button->setOrientation(orientation());
    layout->addWidget(m_button);

    // Instantiate custom ClassicX Menu and pre-initialize geometry so 1st click positions perfectly
    m_menu = new PanelKMenu();
    m_menu->setButton(m_button);
    m_menu->setName("ClassicXMenuWidget");
    m_menu->initialize();
    m_menu->adjustSize();

    m_button->setPopup(m_menu);
    m_button->setInitialized(true);

    // Register button with DCOP MenuManager and replace MenuManager's KMenuStub target with our PanelKMenu.
    // Keep the previous stub target so we can restore it before deleting m_menu (C2: no dangling Alt+F1).
    MenuManager *mgr = findKickerMenuManager();
    if (mgr) {
        mgr->registerKButton(m_button);
        KMenuStub *stubObj = mgr->kmenu();
        if (stubObj) {
            m_menuManagerPrevious = stubObj->panelKMenu();
            // Never treat our own menu as "previous" (re-entrant / double-init safety).
            if (m_menuManagerPrevious == m_menu)
                m_menuManagerPrevious = 0;
            stubObj->setPanelKMenu(m_menu);
        }
    }

    // Schedule singleShot timer to sync icon immediately after Kicker finishes widget placement
    TQTimer::singleShot(0, this, TQT_SLOT(updateIcon()));
}

ClassicXApplet::~ClassicXApplet()
{
    MenuManager *mgr = findKickerMenuManager();
    if (mgr && m_button) {
        mgr->unregisterKButton(m_button);
    }

    // Restore MenuManager stub BEFORE deleting our menu. Only restore if stub still points at us
    // (another party may have replaced it). Never leave a freed PanelKMenu* in the stub.
    if (mgr) {
        KMenuStub *stubObj = mgr->kmenu();
        if (stubObj && stubObj->panelKMenu() == m_menu) {
            stubObj->setPanelKMenu(m_menuManagerPrevious);
        }
    }
    m_menuManagerPrevious = 0;

    delete m_menu;
    m_menu = 0;

    delete m_button;
    m_button = 0;

    KickerTip::cleanUp();

    KickerLib::clearMenuIconSetCache();
}

int ClassicXApplet::widthForHeight(int height) const
{
    m_panelHeight = height;
    return m_button ? m_button->widthForHeight(height) : height;
}

int ClassicXApplet::heightForWidth(int width) const
{
    m_panelHeight = width;
    return m_button ? m_button->heightForWidth(width) : width;
}

void ClassicXApplet::showMenu()
{
    if (!m_button || !m_menu)
        return;

    // Toggle close if menu is already showing
    if (m_menu->isVisible()) {
        m_menu->hide();
        m_button->setDown(false);
        return;
    }

    if (!m_menu->initialized()) {
        m_menu->initialize();
        m_menu->adjustSize();
    }

    m_button->setPopupDirection(popupDirection());
    m_button->setOrientation(orientation());
    m_button->setDrawArrow(true);

    m_button->realShowMenu();
}

void ClassicXApplet::positionChange(Position p)
{
    KPanelApplet::positionChange(p);
    if (m_button)
        m_button->setPopupDirection(popupDirection());
}

void ClassicXApplet::updateIcon()
{
    int pH = panelHeight();
    TQPixmap px = EmbeddedIcons::loadStartMenuIcon(pH);
    if (m_button) {
        m_button->setIconPixmap(px);
        m_button->configure();
        m_button->update();
    }
    emit updateLayout();
}

void ClassicXApplet::preferences()
{
    if (m_menu) {
        m_menu->slotShowClassicXSettings();
    }
}

#include "classicx_applet.moc"
