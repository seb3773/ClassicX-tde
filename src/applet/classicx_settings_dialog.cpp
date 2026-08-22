/*****************************************************************
 * Classic-X settings dialog (extracted from k_mnu — H13).
 *****************************************************************/

#pragma GCC optimize ("Os")

#include "classicx_settings_dialog.h"

#include <tqapplication.h>
#include <tqbuttongroup.h>
#include <tqcheckbox.h>
#include <tqcombobox.h>
#include <tqcolordialog.h>
#include <tqevent.h>
#include <tqfile.h>
#include <tqfont.h>
#include <tqfontdialog.h>
#include <tqframe.h>
#include <tqlabel.h>
#include <tqlayout.h>
#include <tqpainter.h>
#include <tqpen.h>
#include <tqpushbutton.h>
#include <tqradiobutton.h>
#include <tqslider.h>
#include <tqspinbox.h>
#include <tqlineedit.h>
#include <tqimage.h>
#include <tqpixmap.h>
#include <tqtooltip.h>
#include <tqinputdialog.h>
#include <tqmap.h>

#include <tdeapplication.h>
#include <tdeconfig.h>
#include <tdeglobal.h>
#include <tdeglobalsettings.h>
#include <tdelocale.h>
#include <ksimpleconfig.h>
#include <kstandarddirs.h>
#include <kicondialog.h>
#include <tdefiledialog.h>
#include <dcopclient.h>
#include <tdehardwaredevices.h>
#include <tdemessagebox.h>
#include <kdialog.h>

#include "classicxSettings.h"
#include "classicx_profile.h"
#include "classicx_applet.h"
#include "embedded_icons.h"
#include "global.h"
#include "k_mnu.h"
#include "panelbutton.h"
#include "powermanager_flags.h"
#include "recentapps.h"
#include "x11opacity.h"

#ifndef TQT_SIGNAL
#define TQT_SIGNAL TQ_SIGNAL
#endif
#ifndef TQT_SLOT
#define TQT_SLOT TQ_SLOT
#endif

static TQWidget* createSectionLabel(const TQString &title, TQWidget *parent) {
    TQWidget *container = new TQWidget(parent);
    TQHBoxLayout *layout = new TQHBoxLayout(container, 0, 6);

    TQLabel *lbl = new TQLabel(title, container);
    TQFont font = lbl->font();
    font.setBold(true);
    lbl->setFont(font);
    lbl->setAlignment(TQt::AlignLeft | TQt::AlignVCenter);
    layout->addWidget(lbl);

    TQFrame *line = new TQFrame(container);
    line->setFrameShape(TQFrame::HLine);
    line->setFrameShadow(TQFrame::Sunken);
    layout->addWidget(line, 1);

    return container;
}

// MenuEntryHeight: 0 = Classic 20px, -1 = TDE Small, else pixels.
static int treeIconHeightToComboIndex(int h)
{
    if (h < 0)
        return 5;
    if (h == 0 || h == 20)
        return 0;
    if (h <= 16)
        return 1;
    if (h <= 22)
        return 2;
    if (h <= 24)
        return 3;
    return 4;
}

static int treeIconComboIndexToHeight(int idx)
{
    switch (idx) {
    case 1: return 16;
    case 2: return 22;
    case 3: return 24;
    case 4: return 32;
    case 5: return -1;
    default: return 0;
    }
}

// Stored *IconType: 0 = Win (legacy embedded), 1 = Custom (legacy), 2 = KDE.
// Combo order: 0 = Win, 1 = KDE, 2 = Custom.
static int uiIconComboIndex(int storedType)
{
    if (storedType == 2)
        return 1;
    if (storedType == 1)
        return 2;
    return 0;
}

static int uiIconStoredType(int comboIndex)
{
    if (comboIndex == 1)
        return 2;
    if (comboIndex == 2)
        return 1;
    return 0;
}

static int uiIconTypeFromSettings(int index)
{
    switch (index) {
    case 0: return ClassicXSettings::shutdownIconType();
    case 1: return ClassicXSettings::standbyIconType();
    case 2: return ClassicXSettings::logoutIconType();
    case 3: return ClassicXSettings::restartIconType();
    case 4: return ClassicXSettings::hibernateIconType();
    case 5: return ClassicXSettings::hybridSleepIconType();
    case 6: return ClassicXSettings::documentsIconType();
    case 7: return ClassicXSettings::imagesIconType();
    case 8: return ClassicXSettings::settingsIconType();
    default: return 0;
    }
}

static void profilePut(TQMap<TQString, TQString>& m, const char* key, const TQString& value)
{
    m.insert(TQString::fromLatin1(key), value);
}

static void profilePutNum(TQMap<TQString, TQString>& m, const char* key, int value)
{
    profilePut(m, key, TQString::number(value));
}

static void profilePutBool(TQMap<TQString, TQString>& m, const char* key, bool value)
{
    profilePut(m, key, value ? TQString::fromLatin1("true") : TQString::fromLatin1("false"));
}

static void profilePutColor(TQMap<TQString, TQString>& m, const char* key, const TQColor& col)
{
    profilePut(m, key, col.isValid() ? col.name() : TQString::fromLatin1(""));
}

static bool profileHas(const TQMap<TQString, TQString>& m, const char* key)
{
    return m.contains(TQString::fromLatin1(key));
}

static TQString profileGet(const TQMap<TQString, TQString>& m, const char* key)
{
    TQMap<TQString, TQString>::ConstIterator it = m.find(TQString::fromLatin1(key));
    if (it == m.end())
        return TQString::null;
    return it.data();
}

static int profileGetInt(const TQMap<TQString, TQString>& m, const char* key)
{
    return profileGet(m, key).toInt();
}

static bool profileGetBool(const TQMap<TQString, TQString>& m, const char* key)
{
    const TQString v = profileGet(m, key).lower();
    return v == TQString::fromLatin1("true") || v == TQString::fromLatin1("1");
}

static TQColor profileGetColor(const TQMap<TQString, TQString>& m, const char* key)
{
    const TQString v = profileGet(m, key);
    if (v.isEmpty())
        return TQColor();
    return TQColor(v);
}

static void setComboByText(TQComboBox* cmb, const TQString& text)
{
    if (!cmb)
        return;
    for (int i = 0; i < cmb->count(); ++i) {
        if (cmb->text(i) == text) {
            cmb->setCurrentItem(i);
            return;
        }
    }
}

static void setComboIndex(TQComboBox* cmb, int idx)
{
    if (!cmb || cmb->count() <= 0)
        return;
    if (idx < 0)
        idx = 0;
    if (idx >= cmb->count())
        idx = cmb->count() - 1;
    cmb->setCurrentItem(idx);
}

static void connectProfileDirty(TQObject* obj, const char* signal, TQObject* dlg)
{
    if (obj)
        TQObject::connect(obj, signal, dlg, TQT_SLOT(onSettingsEdited()));
}

static TQPixmap uiIconPreviewPixmap(const TQString &embeddedName, int size)
{
    TQImage img = EmbeddedIcons::getNativeImage(embeddedName);
    if (img.isNull())
        return TQPixmap();
    if (img.width() != size || img.height() != size)
        img = img.smoothScale(size, size);
    TQPixmap px;
    px.convertFromImage(img);
    return px;
}

ClassicXSettingsDialog::ClassicXSettingsDialog(TQWidget* parent)
    : TQDialog(parent, "ClassicXSettingsDialog", false)
    , m_cmbProfile(0)
    , m_btnProfileSave(0)
    , m_btnProfileDelete(0)
    , m_ignoreProfileDirty(false)
    , m_updatingSidebarConstraints(false)
{
    setCaption("ClassicX Menu Settings");
    setIcon(EmbeddedIcons::getPixmap("menu-settings", 32, 32));
    resize(1200, 855);

    TQVBoxLayout *mainLayout = new TQVBoxLayout(this, 12, 6);

    TQHBoxLayout *colsLayout = new TQHBoxLayout();
    colsLayout->setSpacing(24);

    TQVBoxLayout *leftCol = new TQVBoxLayout();
    leftCol->setSpacing(4);
    leftCol->addSpacing(6);

    TQFrame *vLine1 = new TQFrame(this);
    vLine1->setFrameShape(TQFrame::VLine);
    vLine1->setFrameShadow(TQFrame::Sunken);

    TQVBoxLayout *midCol = new TQVBoxLayout();
    midCol->setSpacing(4);
    midCol->addSpacing(6);

    TQFrame *vLine2 = new TQFrame(this);
    vLine2->setFrameShape(TQFrame::VLine);
    vLine2->setFrameShadow(TQFrame::Sunken);

    TQVBoxLayout *rightCol = new TQVBoxLayout();
    rightCol->setSpacing(4);
    rightCol->addSpacing(6);

    TQFrame *vLine3 = new TQFrame(this);
    vLine3->setFrameShape(TQFrame::VLine);
    vLine3->setFrameShadow(TQFrame::Sunken);

    TQVBoxLayout *col4 = new TQVBoxLayout();
    col4->setSpacing(4);
    col4->addSpacing(6);

    colsLayout->addLayout(leftCol, 1);
    colsLayout->addWidget(vLine1);
    colsLayout->addLayout(midCol, 0);
    colsLayout->addWidget(vLine2);
    colsLayout->addLayout(rightCol, 1);
    colsLayout->addWidget(vLine3);
    colsLayout->addLayout(col4, 1);

    mainLayout->addLayout(colsLayout);

    // Group 1: General Settings
    leftCol->addWidget(createSectionLabel("General Settings", this));
    leftCol->addSpacing(4);

    TDESharedConfig::Ptr kickerSharedCfg = TDESharedConfig::openConfig("kickerrc");
    kickerSharedCfg->reparseConfiguration();

    TQHBoxLayout *hBoxFormat = new TQHBoxLayout();
    hBoxFormat->setSpacing(6);
    TQLabel *lblFormat = new TQLabel("Entries format:", this);
    m_cmbMenuEntryFormat = new TQComboBox(false, this);
    m_cmbMenuEntryFormat->setSizePolicy(TQSizePolicy::Maximum, TQSizePolicy::Fixed);
    m_cmbMenuEntryFormat->insertItem("Name only");
    m_cmbMenuEntryFormat->insertItem("Name - Description");
    m_cmbMenuEntryFormat->insertItem("Description only");
    m_cmbMenuEntryFormat->insertItem("Description (name)");

    kickerSharedCfg->setGroup("menus");
    TQString fmtStr = kickerSharedCfg->readEntry("MenuEntryFormat", "DescriptionAndName");

    if (fmtStr == "NameOnly") m_cmbMenuEntryFormat->setCurrentItem(0);
    else if (fmtStr == "NameAndDescription") m_cmbMenuEntryFormat->setCurrentItem(1);
    else if (fmtStr == "DescriptionOnly") m_cmbMenuEntryFormat->setCurrentItem(2);
    else m_cmbMenuEntryFormat->setCurrentItem(3);

    hBoxFormat->addWidget(lblFormat);
    hBoxFormat->addWidget(m_cmbMenuEntryFormat);
    hBoxFormat->addStretch(1);
    leftCol->addLayout(hBoxFormat);

    m_chkShowAppIcons = new TQCheckBox("Show application icons", this);
    m_chkShowAppIcons->setChecked(ClassicXSettings::showAppIcons());
    leftCol->addWidget(m_chkShowAppIcons);

    TQHBoxLayout *hBoxTreeIcon = new TQHBoxLayout();
    m_lblTreeIconSize = new TQLabel("Application icon size:", this);
    m_cmbTreeIconSize = new TQComboBox(false, this);
    m_cmbTreeIconSize->insertItem("Classic (20)");
    m_cmbTreeIconSize->insertItem("16");
    m_cmbTreeIconSize->insertItem("22");
    m_cmbTreeIconSize->insertItem("24");
    m_cmbTreeIconSize->insertItem("32");
    m_cmbTreeIconSize->insertItem("System (TDE)");
    m_cmbTreeIconSize->setCurrentItem(
        treeIconHeightToComboIndex(ClassicXSettings::menuEntryHeight()));
    hBoxTreeIcon->addWidget(m_lblTreeIconSize);
    hBoxTreeIcon->addWidget(m_cmbTreeIconSize);
    hBoxTreeIcon->addStretch();
    leftCol->addLayout(hBoxTreeIcon);

    TQHBoxLayout *hBoxAnimSearch = new TQHBoxLayout();
    m_chkAnimateOpening = new TQCheckBox("Animate opening", this);
    m_chkAnimateOpening->setChecked(ClassicXSettings::animateOpening());
    m_chkAlwaysShowSearchBar = new TQCheckBox("Show search field", this);
    m_chkAlwaysShowSearchBar->setChecked(ClassicXSettings::alwaysShowSearchBar());
    hBoxAnimSearch->addWidget(m_chkAnimateOpening);
    hBoxAnimSearch->addSpacing(16);
    hBoxAnimSearch->addWidget(m_chkAlwaysShowSearchBar);
    hBoxAnimSearch->addStretch(1);
    leftCol->addLayout(hBoxAnimSearch);
    onShowAppIconsToggled(m_chkShowAppIcons->isChecked());
    leftCol->addSpacing(8);

    leftCol->addWidget(new TQLabel("Special items:", this));
    kickerSharedCfg->setGroup("menus");
    TQStringList extList = kickerSharedCfg->readListEntry("Extensions");
    if (extList.isEmpty()) {
        extList.append("prefmenu.desktop");
        extList.append("systemmenu.desktop");
    }

    m_chkShowRunCommand = new TQCheckBox("Execute command", this);
    m_chkShowRunCommand->setChecked(ClassicXSettings::showRunCommand());

    m_chkControlCenter = new TQCheckBox("Trinity control center", this);
    m_chkControlCenter->setChecked(ClassicXSettings::showControlCenter());

    m_chkShowBookmarks = new TQCheckBox("Bookmarks", this);
    m_chkShowBookmarks->setChecked(ClassicXSettings::useBookmarks());

    m_chkShowPrintSystem = new TQCheckBox("Print system", this);
    m_chkShowPrintSystem->setChecked(extList.contains("printmenu.desktop"));

    m_chkShowQuickBrowser = new TQCheckBox("Quick browser", this);
    m_chkShowQuickBrowser->setChecked(ClassicXSettings::useBrowser());

    m_chkShowNetworkFolders = new TQCheckBox("Network Folders", this);
    m_chkShowNetworkFolders->setChecked(extList.contains("remotemenu.desktop"));

    m_chkShowSystemMenu = new TQCheckBox("System Menu", this);
    m_chkShowSystemMenu->setChecked(extList.contains("systemmenu.desktop"));

    m_chkShowRecentDocs = new TQCheckBox("Recent documents", this);
    m_chkShowRecentDocs->setChecked(extList.contains("recentdocs.desktop"));

    m_chkShowSpecialUserMenu = new TQCheckBox("User menu", this);
    m_chkShowSpecialUserMenu->setChecked(ClassicXSettings::showSpecialUserMenu());

    m_chkShowSpecialShutdownMenu = new TQCheckBox("Shutdown menu", this);
    m_chkShowSpecialShutdownMenu->setChecked(ClassicXSettings::showSpecialShutdownMenu());

    TQGridLayout *gridSpecial = new TQGridLayout(5, 2, 6);
    gridSpecial->addWidget(m_chkShowRunCommand, 0, 0);
    gridSpecial->addWidget(m_chkShowQuickBrowser, 0, 1);
    gridSpecial->addWidget(m_chkControlCenter, 1, 0);
    gridSpecial->addWidget(m_chkShowNetworkFolders, 1, 1);
    gridSpecial->addWidget(m_chkShowBookmarks, 2, 0);
    gridSpecial->addWidget(m_chkShowSystemMenu, 2, 1);
    gridSpecial->addWidget(m_chkShowPrintSystem, 3, 0);
    gridSpecial->addWidget(m_chkShowRecentDocs, 3, 1);
    gridSpecial->addWidget(m_chkShowSpecialUserMenu, 4, 0);
    gridSpecial->addWidget(m_chkShowSpecialShutdownMenu, 4, 1);

    leftCol->addLayout(gridSpecial);
    leftCol->addSpacing(10);

    bool showApps = ClassicXSettings::showRecentApps();

    TQGridLayout *gridRecent = new TQGridLayout(3, 3, 6);
    gridRecent->setSpacing(8);

    m_chkShowRecentApps = new TQCheckBox("Show", this);
    m_chkShowRecentApps->setChecked(showApps);
    TQToolTip::add(m_chkShowRecentApps, i18n("Default display mode when opening menu (can be toggled temporarily by clicking the section header in the menu)."));

    m_cmbRecentMode = new TQComboBox(false, this);
    m_cmbRecentMode->setSizePolicy(TQSizePolicy::Maximum, TQSizePolicy::Fixed);
    m_cmbRecentMode->insertItem("recents");
    m_cmbRecentMode->insertItem("frequents");
    m_cmbRecentMode->setCurrentItem(ClassicXSettings::recentVsOften() ? 0 : 1);
    TQToolTip::add(m_cmbRecentMode, i18n("Sort by last launch (recents) or by launch count (frequents)."));

    m_lblShownApps = new TQLabel("Applications", this);

    TQHBoxLayout *hBoxShownApps = new TQHBoxLayout();
    hBoxShownApps->setSpacing(4);
    hBoxShownApps->addWidget(m_chkShowRecentApps);
    hBoxShownApps->addWidget(m_cmbRecentMode);
    hBoxShownApps->addWidget(m_lblShownApps);
    hBoxShownApps->addStretch(1);

    int shownCount = ClassicXSettings::numRecentApps();
    if (shownCount < 2)
        shownCount = 2;
    if (shownCount > 6)
        shownCount = 6;
    m_spinNumRecentApps = new TQSpinBox(2, 6, 1, this);
    m_spinNumRecentApps->setValue(shownCount);
    TQToolTip::add(m_spinNumRecentApps, i18n("Number of applications shown in that section."));
    updateShownAppsCountEnabled();

    m_lblMaxRecentDocs = new TQLabel("Maximum number of recent docs", this);
    TDESharedConfig::Ptr kdeGlobalsCfg = TDESharedConfig::openConfig("kdeglobals");
    kdeGlobalsCfg->reparseConfiguration();
    kdeGlobalsCfg->setGroup("RecentDocuments");
    int maxDocs = kdeGlobalsCfg->readNumEntry("MaxEntries", 10);
    m_spinMaxRecentDocs = new TQSpinBox(1, 30, 1, this);
    m_spinMaxRecentDocs->setValue(maxDocs);

    bool recentDocsEnabled = m_chkShowRecentDocs->isChecked();
    m_lblMaxRecentDocs->setEnabled(recentDocsEnabled);
    m_spinMaxRecentDocs->setEnabled(recentDocsEnabled);

    m_lblMaxSearchResults = new TQLabel("Max. Search results", this);
    int maxSearchResults = ClassicXSettings::maxSearchResults();
    if (maxSearchResults < 1) maxSearchResults = 1;
    if (maxSearchResults > 30) maxSearchResults = 30;
    m_spinMaxSearchResults = new TQSpinBox(1, 30, 1, this);
    m_spinMaxSearchResults->setValue(maxSearchResults);

    gridRecent->addLayout(hBoxShownApps, 0, 0);
    gridRecent->addWidget(m_spinNumRecentApps, 0, 1);
    gridRecent->addWidget(m_lblMaxRecentDocs, 1, 0);
    gridRecent->addWidget(m_spinMaxRecentDocs, 1, 1);
    gridRecent->addWidget(m_lblMaxSearchResults, 2, 0);
    gridRecent->addWidget(m_spinMaxSearchResults, 2, 1);
    gridRecent->setColStretch(2, 1);

    leftCol->addLayout(gridRecent);
    leftCol->addSpacing(8);

    TQHBoxLayout *hBoxTrans = new TQHBoxLayout();
    TQLabel *lblTrans = new TQLabel("Transparency:", this);
    m_sliderOpacity = new TQSlider(TQt::Horizontal, this);
    m_sliderOpacity->setRange(0, 80);

    int currentOpacity = ClassicXSettings::classicKMenuOpacity();
    int currentTrans = 100 - currentOpacity;
    if (currentTrans < 0) currentTrans = 0;
    if (currentTrans > 80) currentTrans = 80;

    m_sliderOpacity->setValue(currentTrans);
    m_lblOpacityVal = new TQLabel(TQString::number(currentTrans) + "%", this);
    m_lblOpacityVal->setFixedWidth(36);

    connect(m_sliderOpacity, TQT_SIGNAL(valueChanged(int)), this, TQT_SLOT(onOpacitySliderChanged(int)));

    hBoxTrans->addWidget(lblTrans);
    hBoxTrans->addWidget(m_sliderOpacity);
    hBoxTrans->addWidget(m_lblOpacityVal);
    leftCol->addLayout(hBoxTrans);
    leftCol->addSpacing(4);

    // Group 2: Color Scheme (moved to leftCol)
    leftCol->addWidget(createSectionLabel("Color Scheme", this));
    leftCol->addSpacing(4);

    TQHBoxLayout *hBoxMode = new TQHBoxLayout();
    hBoxMode->addWidget(new TQLabel("Color mode:", this));
    m_cmbColorMode = new TQComboBox(false, this);
    m_cmbColorMode->insertItem("Default");
    m_cmbColorMode->insertItem("TDE System");
    m_cmbColorMode->insertItem("Custom");
    hBoxMode->addWidget(m_cmbColorMode);
    leftCol->addLayout(hBoxMode);
    leftCol->addSpacing(6);

    // Load existing settings
    KSimpleConfig config(TQString::fromLatin1("classicxapplet_rc"));
    config.setGroup("Colors");

    int mode = config.readNumEntry("ColorMode", 0);
    m_cmbColorMode->setCurrentItem(mode);

    m_fgCustom = TQColor(config.readEntry("FgColor", "#000000"));
    m_bgCustom = TQColor(config.readEntry("BgColor", "#F5F6F8"));
    TQString sideBgStr = config.readEntry("SidebarBgColor", "");
    m_sidebarBgCustom = !sideBgStr.isEmpty() ? TQColor(sideBgStr) : TQColor();
    m_textBgCustom = TQColor(config.readEntry("TextBgColor", "#FFFFFF"));
    m_titleFgCustom = TQColor(config.readEntry("TitleFgColor", "#000000"));
    m_titleBgCustom = TQColor(config.readEntry("TitleBgColor", "#E0E4E8"));
    TQString searchTextStr = config.readEntry("SearchTextColor", "");
    m_searchTextCustom = !searchTextStr.isEmpty() ? TQColor(searchTextStr) : m_fgCustom;
    TQString btnHoverStr = config.readEntry("ButtonHoverColor", "");
    m_buttonHoverBgCustom = !btnHoverStr.isEmpty() ? TQColor(btnHoverStr) : TQColor();

    m_btnFg = new TQPushButton(this);
    m_btnFg->setFixedSize(30, 22);
    m_btnBg = new TQPushButton(this);
    m_btnBg->setFixedSize(30, 22);
    m_btnSidebarBg = new TQPushButton(this);
    m_btnSidebarBg->setFixedSize(30, 22);
    m_btnTextBg = new TQPushButton(this);
    m_btnTextBg->setFixedSize(30, 22);
    m_btnTitleFg = new TQPushButton(this);
    m_btnTitleFg->setFixedSize(30, 22);
    m_btnTitleBg = new TQPushButton(this);
    m_btnTitleBg->setFixedSize(30, 22);
    m_btnSearchText = new TQPushButton(this);
    m_btnSearchText->setFixedSize(30, 22);
    m_btnButtonHoverBg = new TQPushButton(this);
    m_btnButtonHoverBg->setFixedSize(30, 22);

    TQHBoxLayout *hBoxColors = new TQHBoxLayout();
    hBoxColors->setSpacing(12);

    TQGridLayout *leftGrid = new TQGridLayout(4, 2, 6);
    leftGrid->addWidget(new TQLabel("Foreground (Fg)", this), 0, 0);
    leftGrid->addWidget(m_btnFg, 0, 1);
    leftGrid->addWidget(new TQLabel("Background (Bg)", this), 1, 0);
    leftGrid->addWidget(m_btnBg, 1, 1);
    leftGrid->addWidget(new TQLabel("Sidebar Bg", this), 2, 0);
    leftGrid->addWidget(m_btnSidebarBg, 2, 1);
    leftGrid->addWidget(new TQLabel("Search Field Bg", this), 3, 0);
    leftGrid->addWidget(m_btnTextBg, 3, 1);

    TQGridLayout *rightGrid = new TQGridLayout(4, 2, 6);
    rightGrid->addWidget(new TQLabel("Title Text", this), 0, 0);
    rightGrid->addWidget(m_btnTitleFg, 0, 1);
    rightGrid->addWidget(new TQLabel("Title Text Bg", this), 1, 0);
    rightGrid->addWidget(m_btnTitleBg, 1, 1);
    rightGrid->addWidget(new TQLabel("Search text", this), 2, 0);
    rightGrid->addWidget(m_btnSearchText, 2, 1);
    rightGrid->addWidget(new TQLabel("Button Hover", this), 3, 0);
    rightGrid->addWidget(m_btnButtonHoverBg, 3, 1);

    hBoxColors->addLayout(leftGrid);
    hBoxColors->addLayout(rightGrid);

    leftCol->addLayout(hBoxColors);
    leftCol->addSpacing(4);

    // Group 3: Font (moved to leftCol)
    leftCol->addWidget(createSectionLabel("Font", this));
    leftCol->addSpacing(4);

    TQHBoxLayout *hBoxFont = new TQHBoxLayout();
    hBoxFont->setSpacing(6);

    // Load font settings
    config.setGroup("Font");
    int fontMode = config.readNumEntry("FontMode", 0);
    TQFont sysFont = TDEGlobalSettings::menuFont();
    TQString fontStr = config.readEntry("Font", sysFont.toString());
    if (fontStr.isEmpty() || !m_customFont.fromString(fontStr)) {
        m_customFont = sysFont;
    }

    m_cmbFontMode = new TQComboBox(false, this);
    m_cmbFontMode->insertItem("Default (TDE)");
    m_cmbFontMode->insertItem("Custom");
    m_cmbFontMode->setCurrentItem(fontMode);

    m_btnChooseFont = new TQPushButton(this);

    hBoxFont->addWidget(m_cmbFontMode);
    hBoxFont->addWidget(m_btnChooseFont, 1);

    leftCol->addLayout(hBoxFont);
    leftCol->addSpacing(4);
    leftCol->addStretch(1);

    // Group 4: Sidebar Picture & Top Picture (in 4th column)
    config.setGroup("Sidebar");
    int sbPicMode = config.readNumEntry("SidebarPictureMode", 0);
    int sbPicSource = config.readNumEntry("SidebarPictureSource", 0);
    TQString sbPicEmb = config.readEntry("SidebarPictureEmbedded", "Chevron");
    m_customPicPath = config.readEntry("SidebarPictureCustomPath", "");
    int sbPicWidthMode = config.readNumEntry("SidebarPictureWidthMode", 0);
    int sbPicAlignMode = config.readNumEntry("SidebarPictureAlignMode", 0);

    col4->addWidget(createSectionLabel("Sidebar Picture", this));
    col4->addSpacing(4);

    TQHBoxLayout *hBoxPicMode = new TQHBoxLayout();
    m_lblPicMode = new TQLabel("Type:", this);
    m_cmbSidebarPicMode = new TQComboBox(false, this);
    m_cmbSidebarPicMode->insertItem("None");
    m_cmbSidebarPicMode->insertItem("Pattern");
    m_cmbSidebarPicMode->insertItem("Picture");
    m_cmbSidebarPicMode->setCurrentItem(sbPicMode);
    hBoxPicMode->addWidget(m_lblPicMode);
    hBoxPicMode->addWidget(m_cmbSidebarPicMode);
    col4->addLayout(hBoxPicMode);

    TQButtonGroup *bgPicSource = new TQButtonGroup(this);
    bgPicSource->hide();
    m_rbPicEmbedded = new TQRadioButton("Embedded", this);
    m_rbPicCustom = new TQRadioButton("Custom", this);
    bgPicSource->insert(m_rbPicEmbedded, 0);
    bgPicSource->insert(m_rbPicCustom, 1);
    if (sbPicSource == 0) m_rbPicEmbedded->setChecked(true);
    else m_rbPicCustom->setChecked(true);

    m_cmbPicEmbedded = new TQComboBox(false, this);
    TQStringList patternList = (sbPicMode == 2) ? EmbeddedIcons::getSidebarPictureNames() : EmbeddedIcons::getSidebarPatternNames();
    for (TQStringList::ConstIterator it = patternList.begin(); it != patternList.end(); ++it) {
        m_cmbPicEmbedded->insertItem(*it);
    }
    int embIndex = patternList.findIndex(sbPicEmb);
    if (embIndex >= 0) m_cmbPicEmbedded->setCurrentItem(embIndex);

    m_btnBrowseCustomPic = new TQPushButton("Browse...", this);

    TQHBoxLayout *hBoxPicEmb = new TQHBoxLayout();
    hBoxPicEmb->addWidget(m_rbPicEmbedded);
    hBoxPicEmb->addWidget(m_cmbPicEmbedded);

    TQHBoxLayout *hBoxPicCust = new TQHBoxLayout();
    hBoxPicCust->addWidget(m_rbPicCustom);
    hBoxPicCust->addWidget(m_btnBrowseCustomPic);

    col4->addLayout(hBoxPicEmb);
    col4->addLayout(hBoxPicCust);

    TQButtonGroup *bgPicWidth = new TQButtonGroup(this);
    bgPicWidth->hide();
    m_rbPicStretch = new TQRadioButton("Stretch", this);
    m_rbPicCrop = new TQRadioButton("Crop", this);
    bgPicWidth->insert(m_rbPicStretch, 0);
    bgPicWidth->insert(m_rbPicCrop, 1);
    if (sbPicWidthMode == 0) m_rbPicStretch->setChecked(true);
    else m_rbPicCrop->setChecked(true);

    TQButtonGroup *bgPicAlign = new TQButtonGroup(this);
    bgPicAlign->hide();
    m_rbPicAlignTop = new TQRadioButton("Align to top", this);
    m_rbPicAlignBottom = new TQRadioButton("Align to bottom", this);
    bgPicAlign->insert(m_rbPicAlignTop, 0);
    bgPicAlign->insert(m_rbPicAlignBottom, 1);
    if (sbPicAlignMode == 0) m_rbPicAlignTop->setChecked(true);
    else m_rbPicAlignBottom->setChecked(true);

    TQHBoxLayout *hBoxWidthMode = new TQHBoxLayout();
    hBoxWidthMode->addWidget(m_rbPicStretch);
    hBoxWidthMode->addWidget(m_rbPicCrop);

    TQHBoxLayout *hBoxAlignMode = new TQHBoxLayout();
    hBoxAlignMode->addWidget(m_rbPicAlignTop);
    hBoxAlignMode->addWidget(m_rbPicAlignBottom);

    m_chkSidebarPicExtendEdges = new TQCheckBox("Extend edge pixels to fill height", this);
    bool sbPicExtendEdges = config.readBoolEntry("SidebarPictureExtendEdges", false);
    m_chkSidebarPicExtendEdges->setChecked(sbPicExtendEdges);

    bool sbPicColorize = config.readBoolEntry("SidebarPictureColorize", false);
    m_sidebarPicColor = TQColor(config.readEntry("SidebarPictureColor",
        TDEGlobalSettings::highlightColor().name()));
    if (!m_sidebarPicColor.isValid())
        m_sidebarPicColor = TDEGlobalSettings::highlightColor();

    TQHBoxLayout *hBoxSbPicColorize = new TQHBoxLayout();
    m_chkSidebarPicColorize = new TQCheckBox("Colorize", this);
    m_chkSidebarPicColorize->setChecked(sbPicColorize);
    hBoxSbPicColorize->addWidget(m_chkSidebarPicColorize);
    hBoxSbPicColorize->addSpacing(8);
    m_btnSidebarPicColor = new TQPushButton("", this);
    m_btnSidebarPicColor->setFixedSize(30, 22);
    updateColorButton(m_btnSidebarPicColor, m_sidebarPicColor);
    m_btnSidebarPicColor->setEnabled(sbPicColorize && sbPicMode != 0);
    hBoxSbPicColorize->addWidget(m_btnSidebarPicColor);
    hBoxSbPicColorize->addStretch(1);

    col4->addLayout(hBoxWidthMode);
    col4->addLayout(hBoxAlignMode);
    col4->addWidget(m_chkSidebarPicExtendEdges);
    col4->addLayout(hBoxSbPicColorize);

    col4->addSpacing(12);

    col4->addWidget(createSectionLabel("Top Picture", this));
    col4->addSpacing(4);

    config.setGroup("KMenu");
    int topPicMode = config.readNumEntry("TopPicMode", 0);
    TQString topPicEmb = config.readEntry("TopPicEmbedded", "Royal");
    m_topPicLeftPath = config.readEntry("TopPicCustomLeft", "");
    m_topPicCenterPath = config.readEntry("TopPicCustomCenter", "");
    m_topPicRightPath = config.readEntry("TopPicCustomRight", "");
    bool topPicColorize = config.readBoolEntry("TopPicColorize", false);

    TQHBoxLayout *hBoxTopMode = new TQHBoxLayout();
    hBoxTopMode->addWidget(new TQLabel("Type:", this));
    m_cmbTopPicMode = new TQComboBox(false, this);
    m_cmbTopPicMode->insertItem("None");
    m_cmbTopPicMode->insertItem("Embedded");
    m_cmbTopPicMode->insertItem("Custom");
    m_cmbTopPicMode->setCurrentItem(topPicMode);
    hBoxTopMode->addWidget(m_cmbTopPicMode);
    col4->addLayout(hBoxTopMode);
    col4->addSpacing(4);

    TQHBoxLayout *hBoxTopEmb = new TQHBoxLayout();
    hBoxTopEmb->addWidget(new TQLabel("Preset theme:", this));
    m_cmbTopPicEmbedded = new TQComboBox(false, this);
    TQStringList topThemes = EmbeddedIcons::getTopPixThemeNames();
    int selIdx = 0;
    for (int i = 0; i < (int)topThemes.count(); ++i) {
        m_cmbTopPicEmbedded->insertItem(topThemes[i]);
        if (topThemes[i].lower() == topPicEmb.lower()) {
            selIdx = i;
        }
    }
    m_cmbTopPicEmbedded->setCurrentItem(selIdx);
    hBoxTopEmb->addWidget(m_cmbTopPicEmbedded);
    col4->addLayout(hBoxTopEmb);
    col4->addSpacing(4);

    TQGridLayout *topCustomGrid = new TQGridLayout(3, 3, 4);

    topCustomGrid->addWidget(new TQLabel("Left part:", this), 0, 0);
    m_lblTopPicLeftPath = new TQLabel(m_topPicLeftPath.isEmpty() ? "None" : m_topPicLeftPath.section('/', -1), this);
    m_btnBrowseTopPicLeft = new TQPushButton("Browse...", this);
    topCustomGrid->addWidget(m_lblTopPicLeftPath, 0, 1);
    topCustomGrid->addWidget(m_btnBrowseTopPicLeft, 0, 2);

    topCustomGrid->addWidget(new TQLabel("Center part:", this), 1, 0);
    m_lblTopPicCenterPath = new TQLabel(m_topPicCenterPath.isEmpty() ? "None" : m_topPicCenterPath.section('/', -1), this);
    m_btnBrowseTopPicCenter = new TQPushButton("Browse...", this);
    topCustomGrid->addWidget(m_lblTopPicCenterPath, 1, 1);
    topCustomGrid->addWidget(m_btnBrowseTopPicCenter, 1, 2);

    topCustomGrid->addWidget(new TQLabel("Right part:", this), 2, 0);
    m_lblTopPicRightPath = new TQLabel(m_topPicRightPath.isEmpty() ? "None" : m_topPicRightPath.section('/', -1), this);
    m_btnBrowseTopPicRight = new TQPushButton("Browse...", this);
    topCustomGrid->addWidget(m_lblTopPicRightPath, 2, 1);
    topCustomGrid->addWidget(m_btnBrowseTopPicRight, 2, 2);

    col4->addLayout(topCustomGrid);
    col4->addSpacing(4);

    TQHBoxLayout *hBoxTopColorize = new TQHBoxLayout();
    m_chkTopPicColorize = new TQCheckBox("Colorize top picture", this);
    m_chkTopPicColorize->setChecked(topPicColorize);
    hBoxTopColorize->addWidget(m_chkTopPicColorize);
    hBoxTopColorize->addSpacing(8);

    m_topPicColor = TQColor(config.readEntry("TopPicColor", TDEGlobalSettings::highlightColor().name()));
    if (!m_topPicColor.isValid()) m_topPicColor = TDEGlobalSettings::highlightColor();

    m_btnTopPicColor = new TQPushButton("", this);
    m_btnTopPicColor->setFixedSize(30, 22);
    updateColorButton(m_btnTopPicColor, m_topPicColor);
    m_btnTopPicColor->setEnabled(topPicColorize && (topPicMode == 1 || topPicMode == 2));
    hBoxTopColorize->addWidget(m_btnTopPicColor);
    hBoxTopColorize->addStretch(1);

    col4->addLayout(hBoxTopColorize);
    col4->addSpacing(4);

    bool topPicShowText = config.readBoolEntry("TopPicShowText", false);
    bool topPicShowUser = config.readBoolEntry("TopPicShowUser", false);
    bool topPicShowCustomText = config.readBoolEntry("TopPicShowCustomText", true);
    TQString topPicText = config.readEntry("TopPicText", "Trinity Desktop");
    int topPicTextColorMode = config.readNumEntry("TopPicTextColorMode", 0);
    TQString topPicTextColorStr = config.readEntry("TopPicTextColor", "");
    m_topPicTextColor = !topPicTextColorStr.isEmpty() ? TQColor(topPicTextColorStr) : TQColor();
    bool topPicShowDate = config.readBoolEntry("TopPicShowDate", false);
    bool topPicShowTime = config.readBoolEntry("TopPicShowTime", false);

    m_chkTopPicShowText = new TQCheckBox("Show text on top picture", this);
    m_chkTopPicShowText->setChecked(topPicShowText);
    col4->addWidget(m_chkTopPicShowText);
    col4->addSpacing(4);

    m_chkTopPicUseUser = new TQCheckBox("User name", this);
    m_chkTopPicUseUser->setChecked(topPicShowUser);
    TQHBoxLayout *hBoxSubUserIndent = new TQHBoxLayout();
    hBoxSubUserIndent->addSpacing(16);
    hBoxSubUserIndent->addWidget(m_chkTopPicUseUser);
    hBoxSubUserIndent->addStretch(1);
    col4->addLayout(hBoxSubUserIndent);
    col4->addSpacing(4);

    m_chkTopPicUseCustom = new TQCheckBox("Custom text", this);
    m_chkTopPicUseCustom->setChecked(topPicShowCustomText);

    TQHBoxLayout *hBoxSubCustomIndent = new TQHBoxLayout();
    hBoxSubCustomIndent->addSpacing(16);
    hBoxSubCustomIndent->addWidget(m_chkTopPicUseCustom);
    hBoxSubCustomIndent->addStretch(1);
    col4->addLayout(hBoxSubCustomIndent);
    col4->addSpacing(2);

    TQHBoxLayout *hBoxTopTextSub = new TQHBoxLayout();
    hBoxTopTextSub->addSpacing(32);
    m_editTopPicText = new TQLineEdit(topPicText, this);
    hBoxTopTextSub->addWidget(m_editTopPicText, 1);
    col4->addLayout(hBoxTopTextSub);
    col4->addSpacing(4);

    m_chkTopPicUseDate = new TQCheckBox("Date of the day", this);
    m_chkTopPicUseDate->setChecked(topPicShowDate);
    TQHBoxLayout *hBoxSubDateIndent = new TQHBoxLayout();
    hBoxSubDateIndent->addSpacing(16);
    hBoxSubDateIndent->addWidget(m_chkTopPicUseDate);
    hBoxSubDateIndent->addStretch(1);
    col4->addLayout(hBoxSubDateIndent);
    col4->addSpacing(4);

    m_chkTopPicUseTime = new TQCheckBox("Time (HH:MM)", this);
    m_chkTopPicUseTime->setChecked(topPicShowTime);
    TQHBoxLayout *hBoxSubTimeIndent = new TQHBoxLayout();
    hBoxSubTimeIndent->addSpacing(16);
    hBoxSubTimeIndent->addWidget(m_chkTopPicUseTime);
    hBoxSubTimeIndent->addStretch(1);
    col4->addLayout(hBoxSubTimeIndent);
    col4->addSpacing(4);

    TQHBoxLayout *hBoxTextColor = new TQHBoxLayout();
    hBoxTextColor->addSpacing(16);
    m_lblTopPicTextColor = new TQLabel("Color:", this);
    m_cmbTopPicTextColorMode = new TQComboBox(false, this);
    m_cmbTopPicTextColorMode->insertItem("TDE");
    m_cmbTopPicTextColorMode->insertItem("Title text color");
    m_cmbTopPicTextColorMode->insertItem("Custom");
    m_cmbTopPicTextColorMode->setCurrentItem(topPicTextColorMode);

    TQColor initDisplayCol;
    if (topPicTextColorMode == 0) {
        initDisplayCol = TQApplication::palette().active().buttonText();
    } else if (topPicTextColorMode == 1) {
        initDisplayCol = m_titleFgCustom;
    } else {
        initDisplayCol = m_topPicTextColor.isValid() ? m_topPicTextColor : m_titleFgCustom;
    }

    m_btnTopPicTextColor = new TQPushButton("", this);
    m_btnTopPicTextColor->setFixedSize(30, 22);
    updateColorButton(m_btnTopPicTextColor, initDisplayCol);

    hBoxTextColor->addWidget(m_lblTopPicTextColor);
    hBoxTextColor->addWidget(m_cmbTopPicTextColorMode);
    hBoxTextColor->addWidget(m_btnTopPicTextColor);
    hBoxTextColor->addStretch(1);
    col4->addLayout(hBoxTextColor);
    col4->addSpacing(6);

    connect(m_cmbTopPicMode, TQT_SIGNAL(activated(int)), this, TQT_SLOT(onTopPicModeChanged(int)));
    connect(m_btnBrowseTopPicLeft, TQT_SIGNAL(clicked()), this, TQT_SLOT(onBrowseTopPicLeftClicked()));
    connect(m_btnBrowseTopPicCenter, TQT_SIGNAL(clicked()), this, TQT_SLOT(onBrowseTopPicCenterClicked()));
    connect(m_btnBrowseTopPicRight, TQT_SIGNAL(clicked()), this, TQT_SLOT(onBrowseTopPicRightClicked()));
    connect(m_chkTopPicColorize, TQT_SIGNAL(toggled(bool)), this, TQT_SLOT(onTopPicColorizeToggled(bool)));
    connect(m_btnTopPicColor, TQT_SIGNAL(clicked()), this, TQT_SLOT(onTopPicColorClicked()));
    connect(m_chkTopPicShowText, TQT_SIGNAL(toggled(bool)), this, TQT_SLOT(onTopPicShowTextToggled(bool)));
    connect(m_chkTopPicUseUser, TQT_SIGNAL(toggled(bool)), this, TQT_SLOT(onTopPicSubTextToggled(bool)));
    connect(m_chkTopPicUseCustom, TQT_SIGNAL(toggled(bool)), this, TQT_SLOT(onTopPicSubTextToggled(bool)));
    connect(m_chkTopPicUseDate, TQT_SIGNAL(toggled(bool)), this, TQT_SLOT(onTopPicSubTextToggled(bool)));
    connect(m_chkTopPicUseTime, TQT_SIGNAL(toggled(bool)), this, TQT_SLOT(onTopPicSubTextToggled(bool)));
    connect(m_cmbTopPicTextColorMode, TQT_SIGNAL(activated(int)), this, TQT_SLOT(onTopPicTextColorModeChanged(int)));
    connect(m_btnTopPicTextColor, TQT_SIGNAL(clicked()), this, TQT_SLOT(onTopPicTextColorClicked()));

    updateTopPictureUI();
    col4->addStretch(1);

    // Group: Menu Icon (moved to midCol top)
    midCol->addWidget(createSectionLabel("Menu Icon", this));
    midCol->addSpacing(4);

    TQHBoxLayout *hBoxPreview = new TQHBoxLayout();
    m_lblStartIconPreview = new TQLabel(this);
    m_lblStartIconPreview->setFrameShape(TQFrame::StyledPanel);
    m_lblStartIconPreview->setFrameShadow(TQFrame::Sunken);
    m_lblStartIconPreview->setAlignment(TQt::AlignCenter);

    hBoxPreview->addWidget(m_lblStartIconPreview);
    hBoxPreview->addStretch(1);

    midCol->addLayout(hBoxPreview);
    midCol->addSpacing(6);

    TQButtonGroup *bgIconType = new TQButtonGroup(this);
    bgIconType->hide();
    bgIconType->setExclusive(true);

    m_rbIconEmbedded = new TQRadioButton("Embedded", this);
    m_rbIconTDE = new TQRadioButton("TDE", this);
    m_rbIconCustom = new TQRadioButton("Custom", this);

    bgIconType->insert(m_rbIconEmbedded, 0);
    bgIconType->insert(m_rbIconTDE, 1);
    bgIconType->insert(m_rbIconCustom, 2);

    TQVBoxLayout *rbBox = new TQVBoxLayout();

    TQHBoxLayout *hBoxEmb = new TQHBoxLayout();
    hBoxEmb->addWidget(m_rbIconEmbedded);
    m_cmbEmbeddedIcon = new TQComboBox(false, this);
    TQStringList startIcons = EmbeddedIcons::getStartIconNames();
    for (TQStringList::ConstIterator it = startIcons.begin(); it != startIcons.end(); ++it) {
        m_cmbEmbeddedIcon->insertItem(*it);
    }
    hBoxEmb->addWidget(m_cmbEmbeddedIcon);
    rbBox->addLayout(hBoxEmb);

    rbBox->addWidget(m_rbIconTDE);

    TQHBoxLayout *hBoxCust = new TQHBoxLayout();
    hBoxCust->addWidget(m_rbIconCustom);
    m_btnBrowseCustomIcon = new TQPushButton("Browse...", this);
    hBoxCust->addWidget(m_btnBrowseCustomIcon);
    rbBox->addLayout(hBoxCust);

    midCol->addLayout(rbBox);

    midCol->addSpacing(6);
    TQHBoxLayout *hBoxScaleInvert = new TQHBoxLayout();

    m_chkFullScaleStartIcon = new TQCheckBox("Full scale", this);
    m_chkFullScaleStartIcon->setChecked(ClassicXSettings::fullScaleStartIcon());
    hBoxScaleInvert->addWidget(m_chkFullScaleStartIcon);
    hBoxScaleInvert->addSpacing(12);

    m_chkInvertStartIcon = new TQCheckBox("Invert icon", this);
    m_chkInvertStartIcon->setChecked(ClassicXSettings::invertStartIcon());
    hBoxScaleInvert->addWidget(m_chkInvertStartIcon);
    hBoxScaleInvert->addStretch(1);

    midCol->addLayout(hBoxScaleInvert);

    TQHBoxLayout *hBoxColorize = new TQHBoxLayout();
    m_chkColorizeStartIcon = new TQCheckBox("Colorize", this);
    m_chkColorizeStartIcon->setChecked(ClassicXSettings::colorizeStartIcon());
    hBoxColorize->addWidget(m_chkColorizeStartIcon);
    hBoxColorize->addSpacing(8);

    m_startIconColor = ClassicXSettings::startIconColor();
    if (!m_startIconColor.isValid()) m_startIconColor = TQColor(0, 0, 0);

    m_btnStartIconColor = new TQPushButton("", this);
    m_btnStartIconColor->setFixedSize(30, 22);
    updateColorButton(m_btnStartIconColor, m_startIconColor);
    m_btnStartIconColor->setEnabled(m_chkColorizeStartIcon->isChecked());
    hBoxColorize->addWidget(m_btnStartIconColor);
    hBoxColorize->addStretch(1);

    midCol->addLayout(hBoxColorize);
    midCol->addSpacing(8);

    // Group: Sidebar
    config.setGroup("Sidebar");

    midCol->addWidget(createSectionLabel("Sidebar", this));
    midCol->addSpacing(4);

    TQHBoxLayout *hBoxSidebarShowWidth = new TQHBoxLayout();
    m_chkShowSidebar = new TQCheckBox("Show sidebar", this);
    m_chkShowSidebar->setChecked(ClassicXSettings::useSidePixmap());

    TQLabel *lblWidth = new TQLabel("Width:", this);
    m_spinWidth = new TQSpinBox(16, 80, 1, this);
    m_spinWidth->setValue(ClassicXSettings::sideBarWidth());

    TQHBoxLayout *hBoxWidthSub = new TQHBoxLayout();
    hBoxWidthSub->setSpacing(4);
    hBoxWidthSub->addWidget(lblWidth);
    hBoxWidthSub->addWidget(m_spinWidth);

    hBoxSidebarShowWidth->addWidget(m_chkShowSidebar);
    hBoxSidebarShowWidth->addStretch(1);
    hBoxSidebarShowWidth->addLayout(hBoxWidthSub);

    midCol->addLayout(hBoxSidebarShowWidth);

    m_chkSidebarHover = new TQCheckBox("Open sidebar menus on hover\n(user/shutdown)", this);
    m_chkSidebarHover->setChecked(ClassicXSettings::sidebarHoverMenu());
    midCol->addWidget(m_chkSidebarHover);

    TQHBoxLayout *hBoxDelay = new TQHBoxLayout();
    m_lblSidebarHoverDelay = new TQLabel("Hover delay:", this);
    m_spinSidebarHoverDelay = new TQSpinBox(100, 1000, 50, this);
    m_spinSidebarHoverDelay->setSuffix(" ms");
    m_spinSidebarHoverDelay->setValue(ClassicXSettings::sidebarHoverDelay());
    hBoxDelay->addWidget(m_lblSidebarHoverDelay);
    hBoxDelay->addWidget(m_spinSidebarHoverDelay);
    midCol->addLayout(hBoxDelay);

    TQHBoxLayout *hBoxAlign = new TQHBoxLayout();
    m_lblSidebarButtonsAlign = new TQLabel("Buttons alignment:", this);
    m_cmbSidebarButtonsAlign = new TQComboBox(false, this);
    m_cmbSidebarButtonsAlign->insertItem("Center");
    m_cmbSidebarButtonsAlign->insertItem("Left");
    m_cmbSidebarButtonsAlign->insertItem("Right");
    m_cmbSidebarButtonsAlign->setCurrentItem(ClassicXSettings::sidebarButtonsAlign());
    hBoxAlign->addWidget(m_lblSidebarButtonsAlign);
    hBoxAlign->addWidget(m_cmbSidebarButtonsAlign);
    midCol->addLayout(hBoxAlign);

    m_lblButtonsHeader = new TQLabel("Buttons:", this);

    TQGridLayout *gridButtons = new TQGridLayout(3, 2, 6);
    m_chkSidebarUserMenu = new TQCheckBox("User Menu", this);
    m_chkSidebarShutdownMenu = new TQCheckBox("Shutdown Menu", this);
    m_chkSidebarSettings = new TQCheckBox("Settings", this);
    m_chkSidebarDocuments = new TQCheckBox("Documents", this);
    m_chkSidebarImages = new TQCheckBox("Images", this);

    m_chkSidebarUserMenu->setChecked(ClassicXSettings::showSidebarUserMenu());
    m_chkSidebarShutdownMenu->setChecked(ClassicXSettings::showSidebarShutdownMenu());
    m_chkSidebarSettings->setChecked(ClassicXSettings::showSidebarSettings());
    m_chkSidebarDocuments->setChecked(ClassicXSettings::showSidebarDocuments());
    m_chkSidebarImages->setChecked(ClassicXSettings::showSidebarImages());

    gridButtons->addWidget(m_chkSidebarUserMenu, 0, 0);
    gridButtons->addWidget(m_chkSidebarShutdownMenu, 0, 1);
    gridButtons->addWidget(m_chkSidebarSettings, 1, 0);
    gridButtons->addWidget(m_chkSidebarDocuments, 1, 1);
    gridButtons->addWidget(m_chkSidebarImages, 2, 0);

    midCol->addWidget(m_lblButtonsHeader);
    midCol->addLayout(gridButtons);

    midCol->addSpacing(8);

    bool fullHeight = ClassicXSettings::fullUserShutdownMenuHeight();
    int customH = ClassicXSettings::customUserShutdownMenuHeight();
    if (customH < 250) customH = 250;

    m_lblUserShutdownHeight = new TQLabel("User & Shutdown panel height:", this);
    midCol->addWidget(m_lblUserShutdownHeight);

    m_rbUserShutdownFullHeight = new TQRadioButton("Full menu height", this);
    m_rbUserShutdownCustomHeight = new TQRadioButton("Custom", this);
    m_spinUserShutdownCustomHeight = new TQSpinBox(250, 2000, 10, this);
    m_spinUserShutdownCustomHeight->setSuffix(" px");
    m_spinUserShutdownCustomHeight->setValue(customH);

    m_rbUserShutdownFullHeight->setChecked(fullHeight);
    m_rbUserShutdownCustomHeight->setChecked(!fullHeight);
    m_spinUserShutdownCustomHeight->setEnabled(!fullHeight);

    TQButtonGroup *bgUserShutdownHeight = new TQButtonGroup(this);
    bgUserShutdownHeight->hide();
    bgUserShutdownHeight->insert(m_rbUserShutdownFullHeight, 0);
    bgUserShutdownHeight->insert(m_rbUserShutdownCustomHeight, 1);

    midCol->addWidget(m_rbUserShutdownFullHeight);

    TQHBoxLayout *hBoxCustomHeight = new TQHBoxLayout();
    hBoxCustomHeight->addWidget(m_rbUserShutdownCustomHeight);
    hBoxCustomHeight->addWidget(m_spinUserShutdownCustomHeight);
    hBoxCustomHeight->addStretch(1);
    midCol->addLayout(hBoxCustomHeight);
    midCol->addSpacing(6);

    connect(m_rbUserShutdownFullHeight, TQT_SIGNAL(toggled(bool)), this, TQT_SLOT(onUserShutdownHeightToggled(bool)));



    bool sideEnabled = m_chkShowSidebar->isChecked();
    bool hoverEnabled = m_chkSidebarHover->isChecked();
    lblWidth->setEnabled(sideEnabled);
    m_spinWidth->setEnabled(sideEnabled);
    m_chkSidebarHover->setEnabled(sideEnabled);
    m_lblSidebarHoverDelay->setEnabled(sideEnabled && hoverEnabled);
    m_spinSidebarHoverDelay->setEnabled(sideEnabled && hoverEnabled);

    if (m_lblSidebarButtonsAlign) m_lblSidebarButtonsAlign->setEnabled(sideEnabled);
    if (m_cmbSidebarButtonsAlign) m_cmbSidebarButtonsAlign->setEnabled(sideEnabled);

    if (m_lblButtonsHeader) m_lblButtonsHeader->setEnabled(sideEnabled);
    if (m_chkSidebarUserMenu) m_chkSidebarUserMenu->setEnabled(sideEnabled);
    if (m_chkSidebarShutdownMenu) m_chkSidebarShutdownMenu->setEnabled(sideEnabled);
    if (m_chkSidebarSettings) m_chkSidebarSettings->setEnabled(sideEnabled);
    if (m_chkSidebarDocuments) m_chkSidebarDocuments->setEnabled(sideEnabled);
    if (m_chkSidebarImages) m_chkSidebarImages->setEnabled(sideEnabled);

    if (m_lblUserShutdownHeight) m_lblUserShutdownHeight->setEnabled(sideEnabled);
    if (m_rbUserShutdownFullHeight) m_rbUserShutdownFullHeight->setEnabled(sideEnabled);
    if (m_rbUserShutdownCustomHeight) m_rbUserShutdownCustomHeight->setEnabled(sideEnabled);
    if (m_spinUserShutdownCustomHeight) m_spinUserShutdownCustomHeight->setEnabled(sideEnabled && !fullHeight);

    updateSidebarPictureUI();

    midCol->addSpacing(4);

    // Group: Shutdown actions (positioned in midCol below Sidebar)
    midCol->addWidget(createSectionLabel("Shutdown actions", this));
    midCol->addSpacing(4);

    bool sysCanPowerOff = true;
    bool sysCanReboot = true;
    bool sysCanSuspend = true;
    bool sysCanHybridSuspend = false;
    bool sysCanHibernate = true;

#if defined(WITH_TDEHWLIB)
    TDERootSystemDevice* rootDevice = TDEGlobal::hardwareDevices() ? TDEGlobal::hardwareDevices()->rootSystemDevice() : 0;
    if (rootDevice) {
        sysCanPowerOff = rootDevice->canPowerOff();
        sysCanReboot = rootDevice->canPowerOff();
        sysCanSuspend = rootDevice->canSuspend();
        sysCanHybridSuspend = rootDevice->canHybridSuspend();
        sysCanHibernate = rootDevice->canHibernate();
    }
#endif

    // Fresh read for settings UI (admin may have changed power-managerrc externally).
    const PowerManagerFlags& pm = powerManagerFlags(/*forceReload=*/true);
    if (pm.disableSuspend) {
        sysCanSuspend = false;
        sysCanHybridSuspend = false;
    }
    if (pm.disableHibernate) {
        sysCanHibernate = false;
        sysCanHybridSuspend = false;
    }

    m_chkShutdownPowerOff = new TQCheckBox("Shut down", this);
    m_chkShutdownReboot = new TQCheckBox("Reboot", this);
    m_chkShutdownSuspend = new TQCheckBox("Suspend", this);
    m_chkShutdownHybridSuspend = new TQCheckBox("Hybrid suspend", this);
    m_chkShutdownHibernate = new TQCheckBox("Hibernate", this);

    m_chkShutdownPowerOff->setEnabled(sysCanPowerOff);
    m_chkShutdownReboot->setEnabled(sysCanReboot);
    m_chkShutdownSuspend->setEnabled(sysCanSuspend);
    m_chkShutdownHybridSuspend->setEnabled(sysCanHybridSuspend);
    m_chkShutdownHibernate->setEnabled(sysCanHibernate);

    m_chkShutdownPowerOff->setChecked(ClassicXSettings::showShutdownPowerOff());
    m_chkShutdownReboot->setChecked(ClassicXSettings::showShutdownReboot());
    m_chkShutdownSuspend->setChecked(ClassicXSettings::showShutdownSuspend());
    m_chkShutdownHybridSuspend->setChecked(ClassicXSettings::showShutdownHybridSuspend());
    m_chkShutdownHibernate->setChecked(ClassicXSettings::showShutdownHibernate());

    TQGridLayout *gridShutdown = new TQGridLayout(3, 2, 6);
    gridShutdown->addWidget(m_chkShutdownPowerOff, 0, 0);
    gridShutdown->addWidget(m_chkShutdownHybridSuspend, 0, 1);
    gridShutdown->addWidget(m_chkShutdownReboot, 1, 0);
    gridShutdown->addWidget(m_chkShutdownHibernate, 1, 1);
    gridShutdown->addWidget(m_chkShutdownSuspend, 2, 0);

    midCol->addLayout(gridShutdown);

    midCol->addStretch(1);

    // Group: UI Icons (Column 3)
    rightCol->addWidget(createSectionLabel("UI icons", this));
    rightCol->addSpacing(4);

    TQHBoxLayout *uiIconBox = new TQHBoxLayout();
    uiIconBox->addWidget(new TQLabel("Icons size:", this));
    m_cmbUiIconSize = new TQComboBox(false, this);
    uiIconBox->addWidget(m_cmbUiIconSize);
    uiIconBox->addStretch(1);
    rightCol->addLayout(uiIconBox);

    updateUiIconSizeCombo(m_spinWidth->value());
    connect(m_spinWidth, TQT_SIGNAL(valueChanged(int)), this, TQT_SLOT(onSidebarWidthChanged(int)));

    rightCol->addSpacing(6);
    TQHBoxLayout *hBoxUiColorize = new TQHBoxLayout();
    m_chkInvertUiIcons = new TQCheckBox("Invert icons", this);
    m_chkInvertUiIcons->setChecked(ClassicXSettings::invertUiIcons());
    hBoxUiColorize->addWidget(m_chkInvertUiIcons);
    hBoxUiColorize->addSpacing(12);

    m_chkColorizeUiIcons = new TQCheckBox("Colorize", this);
    m_chkColorizeUiIcons->setChecked(ClassicXSettings::colorizeUiIcons());
    hBoxUiColorize->addWidget(m_chkColorizeUiIcons);
    hBoxUiColorize->addSpacing(8);

    m_uiIconColor = ClassicXSettings::uiIconColor();
    if (!m_uiIconColor.isValid()) m_uiIconColor = TQColor(0, 0, 0);

    m_btnUiIconColor = new TQPushButton("", this);
    m_btnUiIconColor->setFixedSize(30, 22);
    updateColorButton(m_btnUiIconColor, m_uiIconColor);
    m_btnUiIconColor->setEnabled(m_chkColorizeUiIcons->isChecked());
    hBoxUiColorize->addWidget(m_btnUiIconColor);
    hBoxUiColorize->addStretch(1);

    rightCol->addLayout(hBoxUiColorize);
    rightCol->addSpacing(6);

    TQHBoxLayout *hBoxUiPreset = new TQHBoxLayout();
    hBoxUiPreset->addWidget(new TQLabel("Preset:", this));
    m_cmbUiIconPreset = new TQComboBox(false, this);
    // Index 0 is blank: mixed individual sources (not the "Custom" preset).
    m_cmbUiIconPreset->insertItem(TQString::fromLatin1(""));
    m_cmbUiIconPreset->insertItem("Win");
    m_cmbUiIconPreset->insertItem("KDE");
    TQToolTip::add(m_cmbUiIconPreset, i18n("Apply Win or KDE to every UI icon at once. Clears if any icon differs."));
    hBoxUiPreset->addWidget(m_cmbUiIconPreset);
    hBoxUiPreset->addStretch(1);
    rightCol->addLayout(hBoxUiPreset);

    m_uiIconControls[0].label = "Shutdown";        m_uiIconControls[0].embeddedName = "kickermenu-logout";
    m_uiIconControls[1].label = "Standby";         m_uiIconControls[1].embeddedName = "menu-sleep";
    m_uiIconControls[2].label = "Logout";          m_uiIconControls[2].embeddedName = "menu-logout";
    m_uiIconControls[3].label = "Restart";         m_uiIconControls[3].embeddedName = "menu-restart";
    m_uiIconControls[4].label = "Hibernate";       m_uiIconControls[4].embeddedName = "menu-hibernate";
    m_uiIconControls[5].label = "Hybrid sleep";    m_uiIconControls[5].embeddedName = "menu-hybrid";
    m_uiIconControls[6].label = "Documents folder";m_uiIconControls[6].embeddedName = "menu-docs";
    m_uiIconControls[7].label = "Images folder";   m_uiIconControls[7].embeddedName = "menu-images";
    m_uiIconControls[8].label = "Settings";        m_uiIconControls[8].embeddedName = "menu-settings";

    for (int i = 0; i < UI_ICON_COUNT; ++i) {
        rightCol->addSpacing(8);
        rightCol->addWidget(new TQLabel(m_uiIconControls[i].label, this));
        rightCol->addSpacing(2);

        TQHBoxLayout *box = new TQHBoxLayout();
        box->setSpacing(6);

        m_uiIconControls[i].previewLabel = new TQLabel(this);
        m_uiIconControls[i].previewLabel->setFixedSize(32, 32);
        m_uiIconControls[i].previewLabel->setFrameShape(TQFrame::StyledPanel);
        m_uiIconControls[i].previewLabel->setFrameShadow(TQFrame::Sunken);
        m_uiIconControls[i].previewLabel->setAlignment(TQt::AlignCenter);

        m_uiIconControls[i].cmbSource = new TQComboBox(false, this);
        m_uiIconControls[i].cmbSource->insertItem("Win");
        m_uiIconControls[i].cmbSource->insertItem("KDE");
        m_uiIconControls[i].cmbSource->insertItem("Custom");

        m_uiIconControls[i].btnBrowse = new TQPushButton("Browse...", this);

        box->addWidget(m_uiIconControls[i].previewLabel, 0, TQt::AlignVCenter);
        box->addSpacing(10);
        box->addWidget(m_uiIconControls[i].cmbSource, 0, TQt::AlignVCenter);
        box->addSpacing(4);
        box->addWidget(m_uiIconControls[i].btnBrowse, 0, TQt::AlignVCenter);
        box->addStretch(1);

        rightCol->addLayout(box);
    }

    rightCol->addStretch(1);

    // Read icon configuration
    ClassicXSettings::self()->readConfig();
    int iconType = ClassicXSettings::iconType();
    TQString embeddedIcon = ClassicXSettings::embeddedIcon();
    m_customIconPath = ClassicXSettings::customIconPath();

    m_uiIconControls[0].customPath = ClassicXSettings::shutdownCustomIconPath();
    m_uiIconControls[1].customPath = ClassicXSettings::standbyCustomIconPath();
    m_uiIconControls[2].customPath = ClassicXSettings::logoutCustomIconPath();
    m_uiIconControls[3].customPath = ClassicXSettings::restartCustomIconPath();
    m_uiIconControls[4].customPath = ClassicXSettings::hibernateCustomIconPath();
    m_uiIconControls[5].customPath = ClassicXSettings::hybridSleepCustomIconPath();
    m_uiIconControls[6].customPath = ClassicXSettings::documentsCustomIconPath();
    m_uiIconControls[7].customPath = ClassicXSettings::imagesCustomIconPath();
    m_uiIconControls[8].customPath = ClassicXSettings::settingsCustomIconPath();
    for (int i = 0; i < UI_ICON_COUNT; ++i) {
        if (m_uiIconControls[i].cmbSource)
            m_uiIconControls[i].cmbSource->setCurrentItem(
                uiIconComboIndex(uiIconTypeFromSettings(i)));
    }
    syncUiIconPresetCombo();

    if (iconType == 1) {
        m_rbIconTDE->setChecked(true);
    } else if (iconType == 2) {
        m_rbIconCustom->setChecked(true);
    } else {
        m_rbIconEmbedded->setChecked(true);
    }

    int embIdx = -1;
    for (int i = 0; i < m_cmbEmbeddedIcon->count(); ++i) {
        if (m_cmbEmbeddedIcon->text(i) == embeddedIcon) {
            embIdx = i;
            break;
        }
    }
    if (embIdx >= 0) {
        m_cmbEmbeddedIcon->setCurrentItem(embIdx);
    }

    connect(bgIconType, TQT_SIGNAL(clicked(int)), this, TQT_SLOT(onIconTypeChanged()));
    connect(m_cmbEmbeddedIcon, TQT_SIGNAL(activated(int)), this, TQT_SLOT(onEmbeddedIconChanged(int)));
    connect(m_btnBrowseCustomIcon, TQT_SIGNAL(clicked()), this, TQT_SLOT(onBrowseCustomIconClicked()));
    connect(m_chkFullScaleStartIcon, TQT_SIGNAL(toggled(bool)), this, TQT_SLOT(onIconTypeChanged()));
    connect(m_chkInvertStartIcon, TQT_SIGNAL(toggled(bool)), this, TQT_SLOT(onInvertStartIconToggled(bool)));
    connect(m_chkColorizeStartIcon, TQT_SIGNAL(toggled(bool)), this, TQT_SLOT(onColorizeStartIconToggled(bool)));
    connect(m_btnStartIconColor, TQT_SIGNAL(clicked()), this, TQT_SLOT(onStartIconColorClicked()));

    connect(m_chkInvertUiIcons, TQT_SIGNAL(toggled(bool)), this, TQT_SLOT(onInvertUiIconsToggled(bool)));
    connect(m_chkColorizeUiIcons, TQT_SIGNAL(toggled(bool)), this, TQT_SLOT(onColorizeUiIconsToggled(bool)));
    connect(m_btnUiIconColor, TQT_SIGNAL(clicked()), this, TQT_SLOT(onUiIconColorClicked()));
    connect(m_cmbUiIconPreset, TQT_SIGNAL(activated(int)), this, TQT_SLOT(onUiIconPresetChanged(int)));

    for (int i = 0; i < UI_ICON_COUNT; ++i) {
        connect(m_uiIconControls[i].cmbSource, TQT_SIGNAL(activated(int)), this, TQT_SLOT(onUiIconTypeChanged()));
        connect(m_uiIconControls[i].btnBrowse, TQT_SIGNAL(clicked()), this, TQT_SLOT(onBrowseUiIconClicked()));
    }

    updateIconPreview();
    updateUiIconPreviews();

    TQFrame *btnSep = new TQFrame(this);
    btnSep->setFrameShape(TQFrame::HLine);
    btnSep->setFrameShadow(TQFrame::Sunken);
    mainLayout->addSpacing(4);
    mainLayout->addWidget(btnSep);

    // Bottom buttons (Left: Edit Menu, Right: OK / Cancel)
    TQHBoxLayout *btnLayout = new TQHBoxLayout();

    TQPushButton *btnEditMenu = new TQPushButton(i18n("&Menu Editor"), this);
    connect(btnEditMenu, TQT_SIGNAL(clicked()), this, TQT_SLOT(onEditMenuClicked()));
    btnLayout->addWidget(btnEditMenu);

    TQPushButton *btnAbout = new TQPushButton(i18n("About"), this);
    connect(btnAbout, TQT_SIGNAL(clicked()), this, TQT_SLOT(onAboutClicked()));
    btnLayout->addWidget(btnAbout);

    btnLayout->addStretch(1);

    TQLabel *lblProfile = new TQLabel(i18n("Profile:"), this);
    m_cmbProfile = new TQComboBox(false, this);
    m_cmbProfile->setMinimumWidth(140);
    m_cmbProfile->setMaximumWidth(220);
    m_btnProfileSave = new TQPushButton(i18n("Save"), this);
    m_btnProfileDelete = new TQPushButton(i18n("Delete"), this);
    connect(m_cmbProfile, TQT_SIGNAL(activated(int)), this, TQT_SLOT(onProfileActivated(int)));
    connect(m_btnProfileSave, TQT_SIGNAL(clicked()), this, TQT_SLOT(onProfileSaveClicked()));
    connect(m_btnProfileDelete, TQT_SIGNAL(clicked()), this, TQT_SLOT(onProfileDeleteClicked()));
    btnLayout->addWidget(lblProfile);
    btnLayout->addWidget(m_cmbProfile);
    btnLayout->addWidget(m_btnProfileSave);
    btnLayout->addWidget(m_btnProfileDelete);

    btnLayout->addStretch(1);

    TQPushButton *btnOk = new TQPushButton("OK", this);
    TQPushButton *btnCancel = new TQPushButton("Cancel", this);
    btnOk->setDefault(true);
    btnOk->setFocus();
    btnLayout->addWidget(btnOk);
    btnLayout->addWidget(btnCancel);

    mainLayout->addLayout(btnLayout);

    // Connections
    connect(m_chkShowSidebar, TQT_SIGNAL(toggled(bool)), this, TQT_SLOT(onShowSidebarToggled(bool)));
    connect(m_chkSidebarHover, TQT_SIGNAL(toggled(bool)), this, TQT_SLOT(onSidebarHoverToggled(bool)));
    connect(m_cmbSidebarPicMode, TQT_SIGNAL(activated(int)), this, TQT_SLOT(onSidebarPicModeChanged(int)));
    connect(m_rbPicEmbedded, TQT_SIGNAL(toggled(bool)), this, TQT_SLOT(onSidebarPicSourceToggled(bool)));
    connect(m_rbPicCustom, TQT_SIGNAL(toggled(bool)), this, TQT_SLOT(onSidebarPicSourceToggled(bool)));
    connect(m_btnBrowseCustomPic, TQT_SIGNAL(clicked()), this, TQT_SLOT(onBrowseCustomPicClicked()));
    connect(m_chkSidebarPicColorize, TQT_SIGNAL(toggled(bool)), this, TQT_SLOT(onSidebarPicColorizeToggled(bool)));
    connect(m_btnSidebarPicColor, TQT_SIGNAL(clicked()), this, TQT_SLOT(onSidebarPicColorClicked()));
    connect(m_chkShowAppIcons, TQT_SIGNAL(toggled(bool)), this, TQT_SLOT(onShowAppIconsToggled(bool)));
    connect(m_chkShowRecentApps, TQT_SIGNAL(toggled(bool)), this, TQT_SLOT(onShowRecentAppsToggled(bool)));
    connect(m_chkShowRecentDocs, TQT_SIGNAL(toggled(bool)), this, TQT_SLOT(onShowRecentDocsToggled(bool)));

    connect(m_cmbColorMode, TQT_SIGNAL(activated(int)), this, TQT_SLOT(onColorModeChanged(int)));

    connect(m_btnFg, TQT_SIGNAL(clicked()), this, TQT_SLOT(onColorButtonClicked()));
    connect(m_btnBg, TQT_SIGNAL(clicked()), this, TQT_SLOT(onColorButtonClicked()));
    connect(m_btnSidebarBg, TQT_SIGNAL(clicked()), this, TQT_SLOT(onColorButtonClicked()));
    connect(m_btnTextBg, TQT_SIGNAL(clicked()), this, TQT_SLOT(onColorButtonClicked()));
    connect(m_btnTitleFg, TQT_SIGNAL(clicked()), this, TQT_SLOT(onColorButtonClicked()));
    connect(m_btnTitleBg, TQT_SIGNAL(clicked()), this, TQT_SLOT(onColorButtonClicked()));
    connect(m_btnSearchText, TQT_SIGNAL(clicked()), this, TQT_SLOT(onColorButtonClicked()));
    connect(m_btnButtonHoverBg, TQT_SIGNAL(clicked()), this, TQT_SLOT(onColorButtonClicked()));

    m_btnFg->installEventFilter(this);
    m_btnBg->installEventFilter(this);
    m_btnSidebarBg->installEventFilter(this);
    m_btnTextBg->installEventFilter(this);
    m_btnTitleFg->installEventFilter(this);
    m_btnTitleBg->installEventFilter(this);
    m_btnSearchText->installEventFilter(this);
    m_btnButtonHoverBg->installEventFilter(this);

    connect(m_cmbFontMode, TQT_SIGNAL(activated(int)), this, TQT_SLOT(onFontModeChanged(int)));
    connect(m_btnChooseFont, TQT_SIGNAL(clicked()), this, TQT_SLOT(onChooseFontClicked()));

    connect(btnOk, TQT_SIGNAL(clicked()), this, TQT_SLOT(onOkClicked()));
    connect(btnCancel, TQT_SIGNAL(clicked()), this, TQT_SLOT(onCancelClicked()));

    connect(m_chkShowSidebar, TQT_SIGNAL(toggled(bool)), this, TQT_SLOT(updateSpecialItemsEnableState()));
    connect(m_chkSidebarUserMenu, TQT_SIGNAL(toggled(bool)), this, TQT_SLOT(updateSpecialItemsEnableState()));
    connect(m_chkSidebarShutdownMenu, TQT_SIGNAL(toggled(bool)), this, TQT_SLOT(updateSpecialItemsEnableState()));
    connect(m_chkSidebarUserMenu, TQT_SIGNAL(toggled(bool)), this, TQT_SLOT(updateSidebarWidthConstraints()));
    connect(m_chkSidebarShutdownMenu, TQT_SIGNAL(toggled(bool)), this, TQT_SLOT(updateSidebarWidthConstraints()));
    connect(m_chkSidebarSettings, TQT_SIGNAL(toggled(bool)), this, TQT_SLOT(updateSidebarWidthConstraints()));
    connect(m_chkSidebarDocuments, TQT_SIGNAL(toggled(bool)), this, TQT_SLOT(updateSidebarWidthConstraints()));
    connect(m_chkSidebarImages, TQT_SIGNAL(toggled(bool)), this, TQT_SLOT(updateSidebarWidthConstraints()));
    connect(m_cmbUiIconSize, TQT_SIGNAL(activated(int)), this, TQT_SLOT(updateSidebarWidthConstraints()));

    // Refresh color button previews and font UI
    refreshColorButtons();
    updateFontUI();
    updateSpecialItemsEnableState();

    connectProfileDirtyTracking();
    updateSidebarWidthConstraints();
    refreshProfileList();
    selectMatchingProfile();
    updateProfileButtons();

    // Set 1200x855 size and center dialog on screen
    setMinimumSize(1100, 780);
    resize(1200, 855);
    KDialog::centerOnScreen(this);
}

void ClassicXSettingsDialog::updateSpecialItemsEnableState()
{
    bool sidebarActive = m_chkShowSidebar->isChecked();
    bool userInSidebar = sidebarActive && m_chkSidebarUserMenu->isChecked();
    bool shutdownInSidebar = sidebarActive && m_chkSidebarShutdownMenu->isChecked();

    if (m_chkShowSpecialUserMenu) {
        m_chkShowSpecialUserMenu->setEnabled(!userInSidebar);
    }
    if (m_chkShowSpecialShutdownMenu) {
        m_chkShowSpecialShutdownMenu->setEnabled(!shutdownInSidebar);
    }
}

ClassicXSettingsDialog::~ClassicXSettingsDialog()
{
}

bool ClassicXSettingsDialog::eventFilter(TQObject* watched, TQEvent* e)
{
    if (watched && watched->inherits("TQPushButton")) {
        TQPushButton* btn = (TQPushButton*)watched;
        if (btn == m_btnFg || btn == m_btnBg || btn == m_btnSidebarBg || btn == m_btnTextBg || btn == m_btnTitleFg || btn == m_btnTitleBg || btn == m_btnSearchText || btn == m_btnButtonHoverBg) {
            if (m_cmbColorMode->currentItem() != 2) {
                // In Default or TDE System mode: ignore hover and click events
                if (e->type() == TQEvent::MouseButtonPress ||
                    e->type() == TQEvent::MouseButtonRelease ||
                    e->type() == TQEvent::MouseButtonDblClick ||
                    e->type() == TQEvent::Enter ||
                    e->type() == TQEvent::Leave) {
                    return true; // Discard event
                }
            }
        }
    }
    return TQDialog::eventFilter(watched, e);
}

void ClassicXSettingsDialog::updateColorButton(TQPushButton* btn, const TQColor& col)
{
    int w = 22;
    int h = 14;
    TQPixmap px(w, h);
    TQPainter p(&px);
    p.fillRect(0, 0, w, h, col);
    p.setPen(TQPen(TQColor(40, 40, 40), 1));
    p.drawRect(0, 0, w, h);
    p.setPen(TQPen(TQColor(210, 210, 210), 1));
    p.drawRect(1, 1, w - 2, h - 2);
    p.end();
    btn->setPixmap(px);
    btn->setEnabled(true);
}

void ClassicXSettingsDialog::refreshColorButtons()
{
    int mode = m_cmbColorMode->currentItem();

    TQColor fg, bg, sidebarBg, textBg, titleFg, titleBg, searchText, buttonHoverBg;

    if (mode == 0) { // Default
        fg = TQColor(0, 0, 0);
        bg = TQColor(245, 246, 248);
        sidebarBg = TQColor(245, 246, 248);
        textBg = TQColor(255, 255, 255);
        titleFg = TQColor(0, 0, 0);
        titleBg = TQColor(220, 224, 230);
        searchText = TQColor(0, 0, 0);
        buttonHoverBg = TQColor(225, 230, 238);
    } else if (mode == 1) { // TDE System
        fg = TQApplication::palette().active().text();
        bg = TQApplication::palette().active().background();
        sidebarBg = TQApplication::palette().active().background();
        textBg = TQApplication::palette().active().base();
        titleFg = TQApplication::palette().active().buttonText();
        titleBg = TQApplication::palette().active().button();
        searchText = TQApplication::palette().active().text();
        buttonHoverBg = TQApplication::palette().active().highlight();
    } else { // Custom
        fg = m_fgCustom;
        bg = m_bgCustom;
        sidebarBg = m_sidebarBgCustom.isValid() ? m_sidebarBgCustom : bg;
        textBg = m_textBgCustom;
        titleFg = m_titleFgCustom;
        titleBg = m_titleBgCustom;
        searchText = m_searchTextCustom.isValid() ? m_searchTextCustom : fg;
        buttonHoverBg = m_buttonHoverBgCustom.isValid() ? m_buttonHoverBgCustom : TQColor(225, 230, 238);
    }

    updateColorButton(m_btnFg, fg);
    updateColorButton(m_btnBg, bg);
    updateColorButton(m_btnSidebarBg, sidebarBg);
    updateColorButton(m_btnTextBg, textBg);
    updateColorButton(m_btnTitleFg, titleFg);
    updateColorButton(m_btnTitleBg, titleBg);
    updateColorButton(m_btnSearchText, searchText);
    updateColorButton(m_btnButtonHoverBg, buttonHoverBg);
}

void ClassicXSettingsDialog::onColorModeChanged(int)
{
    refreshColorButtons();
}

void ClassicXSettingsDialog::onColorButtonClicked()
{
    if (m_cmbColorMode->currentItem() != 2) return;

    TQPushButton* btn = (TQPushButton*)sender();
    if (!btn) return;

    TQColor* customCol = 0;
    if (btn == m_btnFg) customCol = &m_fgCustom;
    else if (btn == m_btnBg) customCol = &m_bgCustom;
    else if (btn == m_btnSidebarBg) customCol = &m_sidebarBgCustom;
    else if (btn == m_btnTextBg) customCol = &m_textBgCustom;
    else if (btn == m_btnTitleFg) customCol = &m_titleFgCustom;
    else if (btn == m_btnTitleBg) customCol = &m_titleBgCustom;
    else if (btn == m_btnSearchText) customCol = &m_searchTextCustom;
    else if (btn == m_btnButtonHoverBg) customCol = &m_buttonHoverBgCustom;

    if (customCol) {
        TQColor initialCol = customCol->isValid() ? *customCol : TQColor(220, 224, 230);
        TQColor c = TQColorDialog::getColor(initialCol, this);
        if (c.isValid()) {
            *customCol = c;
            refreshColorButtons();
            onSettingsEdited();
        }
    }
}

void ClassicXSettingsDialog::updateFontUI()
{
    int fontMode = m_cmbFontMode->currentItem();
    if (fontMode == 0) { // Default (TDE)
        m_btnChooseFont->setEnabled(false);
        TQFont sysFont = TDEGlobalSettings::menuFont();
        m_btnChooseFont->setText(sysFont.family() + ", " + TQString::number(sysFont.pointSize()) + "pt");
    } else { // Custom
        m_btnChooseFont->setEnabled(true);
        m_btnChooseFont->setText(m_customFont.family() + ", " + TQString::number(m_customFont.pointSize()) + "pt");
    }
}

void ClassicXSettingsDialog::onFontModeChanged(int)
{
    updateFontUI();
}

void ClassicXSettingsDialog::onChooseFontClicked()
{
    if (m_cmbFontMode->currentItem() != 1) return;

    bool ok = false;
    TQFont chosen = TQFontDialog::getFont(&ok, m_customFont, this);
    if (ok) {
        m_customFont = chosen;
        updateFontUI();
        onSettingsEdited();
    }
}

void ClassicXSettingsDialog::updateTopPictureUI()
{
    int mode = m_cmbTopPicMode ? m_cmbTopPicMode->currentItem() : 0; // 0 = None, 1 = Embedded, 2 = Custom

    if (m_cmbTopPicEmbedded) {
        m_cmbTopPicEmbedded->setEnabled(mode == 1);
    }

    bool isCustom = (mode == 2);
    if (m_lblTopPicLeftPath) m_lblTopPicLeftPath->setEnabled(isCustom);
    if (m_btnBrowseTopPicLeft) m_btnBrowseTopPicLeft->setEnabled(isCustom);
    if (m_lblTopPicCenterPath) m_lblTopPicCenterPath->setEnabled(isCustom);
    if (m_btnBrowseTopPicCenter) m_btnBrowseTopPicCenter->setEnabled(isCustom);
    if (m_lblTopPicRightPath) m_lblTopPicRightPath->setEnabled(isCustom);
    if (m_btnBrowseTopPicRight) m_btnBrowseTopPicRight->setEnabled(isCustom);

    bool topActive = (mode == 1 || mode == 2);
    if (m_chkTopPicColorize) {
        m_chkTopPicColorize->setEnabled(topActive);
    }
    if (m_btnTopPicColor) {
        m_btnTopPicColor->setEnabled(topActive && m_chkTopPicColorize->isChecked());
    }

    bool showText = topActive && m_chkTopPicShowText && m_chkTopPicShowText->isChecked();
    if (m_chkTopPicShowText) m_chkTopPicShowText->setEnabled(topActive);
    if (m_chkTopPicUseUser) m_chkTopPicUseUser->setEnabled(showText);
    if (m_chkTopPicUseCustom) m_chkTopPicUseCustom->setEnabled(showText);
    if (m_chkTopPicUseDate) m_chkTopPicUseDate->setEnabled(showText);
    if (m_chkTopPicUseTime) m_chkTopPicUseTime->setEnabled(showText);

    bool showCustomFields = showText && m_chkTopPicUseCustom && m_chkTopPicUseCustom->isChecked();
    if (m_editTopPicText) m_editTopPicText->setEnabled(showCustomFields);

    int colorMode = m_cmbTopPicTextColorMode ? m_cmbTopPicTextColorMode->currentItem() : 0;
    if (m_lblTopPicTextColor) m_lblTopPicTextColor->setEnabled(showText);
    if (m_cmbTopPicTextColorMode) m_cmbTopPicTextColorMode->setEnabled(showText);
    if (m_btnTopPicTextColor) m_btnTopPicTextColor->setEnabled(showText && colorMode == 2);
}

void ClassicXSettingsDialog::onTopPicModeChanged(int)
{
    updateTopPictureUI();
}

void ClassicXSettingsDialog::onTopPicColorizeToggled(bool enabled)
{
    int mode = m_cmbTopPicMode ? m_cmbTopPicMode->currentItem() : 0;
    bool topActive = (mode == 1 || mode == 2);
    if (m_btnTopPicColor) {
        m_btnTopPicColor->setEnabled(topActive && enabled);
    }
}

void ClassicXSettingsDialog::onTopPicColorClicked()
{
    if (!m_chkTopPicColorize || !m_chkTopPicColorize->isChecked()) return;

    TQColor col = TQColorDialog::getColor(m_topPicColor, this);
    if (col.isValid()) {
        m_topPicColor = col;
        updateColorButton(m_btnTopPicColor, m_topPicColor);
        onSettingsEdited();
    }
}

void ClassicXSettingsDialog::onTopPicShowTextToggled(bool)
{
    updateTopPictureUI();
}

void ClassicXSettingsDialog::onTopPicSubTextToggled(bool)
{
    if (m_chkTopPicShowText && m_chkTopPicShowText->isChecked()) {
        int checkedCount = 0;
        if (m_chkTopPicUseUser && m_chkTopPicUseUser->isChecked()) checkedCount++;
        if (m_chkTopPicUseCustom && m_chkTopPicUseCustom->isChecked()) checkedCount++;
        if (m_chkTopPicUseDate && m_chkTopPicUseDate->isChecked()) checkedCount++;
        if (m_chkTopPicUseTime && m_chkTopPicUseTime->isChecked()) checkedCount++;

        if (checkedCount == 0) {
            TQObject *snd = const_cast<TQObject*>(sender());
            if (snd && snd->isA("TQCheckBox")) {
                static_cast<TQCheckBox*>(snd)->blockSignals(true);
                static_cast<TQCheckBox*>(snd)->setChecked(true);
                static_cast<TQCheckBox*>(snd)->blockSignals(false);
            } else if (m_chkTopPicUseCustom) {
                m_chkTopPicUseCustom->blockSignals(true);
                m_chkTopPicUseCustom->setChecked(true);
                m_chkTopPicUseCustom->blockSignals(false);
            }
        }
    }
    updateTopPictureUI();
}

void ClassicXSettingsDialog::onTopPicTextColorModeChanged(int index)
{
    if (m_btnTopPicTextColor) {
        TQColor displayCol;
        if (index == 0) {
            displayCol = TQApplication::palette().active().buttonText();
        } else if (index == 1) {
            displayCol = m_titleFgCustom;
        } else {
            displayCol = m_topPicTextColor.isValid() ? m_topPicTextColor : m_titleFgCustom;
        }
        updateColorButton(m_btnTopPicTextColor, displayCol);
    }
    updateTopPictureUI();
}

void ClassicXSettingsDialog::onTopPicTextColorClicked()
{
    if (!m_chkTopPicShowText || !m_chkTopPicShowText->isChecked()) return;
    if (!m_cmbTopPicTextColorMode || m_cmbTopPicTextColorMode->currentItem() != 2) return;

    TQColor defaultColor = m_topPicTextColor.isValid() ? m_topPicTextColor : m_titleFgCustom;
    TQColor col = TQColorDialog::getColor(defaultColor, this);
    if (col.isValid()) {
        m_topPicTextColor = col;
        updateColorButton(m_btnTopPicTextColor, m_topPicTextColor);
        onSettingsEdited();
    }
}

void ClassicXSettingsDialog::onBrowseTopPicLeftClicked()
{
    TQString fn = KFileDialog::getOpenFileName(m_topPicLeftPath, "image/png image/jpeg image/x-xpixmap", this, "Select Top Picture Left Part");
    if (!fn.isEmpty()) {
        m_topPicLeftPath = fn;
        m_lblTopPicLeftPath->setText(fn.section('/', -1));
        onSettingsEdited();
    }
}

void ClassicXSettingsDialog::onBrowseTopPicCenterClicked()
{
    TQString fn = KFileDialog::getOpenFileName(m_topPicCenterPath, "image/png image/jpeg image/x-xpixmap", this, "Select Top Picture Center Part");
    if (!fn.isEmpty()) {
        m_topPicCenterPath = fn;
        m_lblTopPicCenterPath->setText(fn.section('/', -1));
        onSettingsEdited();
    }
}

void ClassicXSettingsDialog::onBrowseTopPicRightClicked()
{
    TQString fn = KFileDialog::getOpenFileName(m_topPicRightPath, "image/png image/jpeg image/x-xpixmap", this, "Select Top Picture Right Part");
    if (!fn.isEmpty()) {
        m_topPicRightPath = fn;
        m_lblTopPicRightPath->setText(fn.section('/', -1));
        onSettingsEdited();
    }
}

void ClassicXSettingsDialog::updateSidebarPictureUI()
{
    bool sideEnabled = m_chkShowSidebar->isChecked();
    int mode = m_cmbSidebarPicMode->currentItem(); // 0 = None, 1 = Pattern, 2 = Picture
    bool isCustom = m_rbPicCustom->isChecked();

    if (m_lblPicMode) m_lblPicMode->setEnabled(sideEnabled);
    if (m_cmbSidebarPicMode) m_cmbSidebarPicMode->setEnabled(sideEnabled);

    if (m_cmbPicEmbedded && mode != 0) {
        TQString currentSel = m_cmbPicEmbedded->currentText();
        m_cmbPicEmbedded->clear();
        TQStringList items = (mode == 2) ? EmbeddedIcons::getSidebarPictureNames() : EmbeddedIcons::getSidebarPatternNames();
        for (TQStringList::ConstIterator it = items.begin(); it != items.end(); ++it) {
            m_cmbPicEmbedded->insertItem(*it);
        }
        int idx = items.findIndex(currentSel);
        if (idx >= 0) {
            m_cmbPicEmbedded->setCurrentItem(idx);
        } else if (m_cmbPicEmbedded->count() > 0) {
            m_cmbPicEmbedded->setCurrentItem(0);
        }
    }

    bool picActive = sideEnabled && mode != 0;
    if (m_chkSidebarPicColorize)
        m_chkSidebarPicColorize->setEnabled(picActive);
    if (m_btnSidebarPicColor)
        m_btnSidebarPicColor->setEnabled(picActive && m_chkSidebarPicColorize &&
                                         m_chkSidebarPicColorize->isChecked());

    if (!sideEnabled || mode == 0) { // None
        if (m_rbPicEmbedded) m_rbPicEmbedded->setEnabled(false);
        if (m_cmbPicEmbedded) m_cmbPicEmbedded->setEnabled(false);
        if (m_rbPicCustom) m_rbPicCustom->setEnabled(false);
        if (m_btnBrowseCustomPic) m_btnBrowseCustomPic->setEnabled(false);
        if (m_rbPicStretch) m_rbPicStretch->setEnabled(false);
        if (m_rbPicCrop) m_rbPicCrop->setEnabled(false);
        if (m_rbPicAlignTop) m_rbPicAlignTop->setEnabled(false);
        if (m_rbPicAlignBottom) m_rbPicAlignBottom->setEnabled(false);
        if (m_chkSidebarPicExtendEdges) m_chkSidebarPicExtendEdges->setEnabled(false);
    } else if (mode == 1) { // Pattern
        if (m_rbPicEmbedded) m_rbPicEmbedded->setEnabled(true);
        if (m_cmbPicEmbedded) m_cmbPicEmbedded->setEnabled(!isCustom);
        if (m_rbPicCustom) m_rbPicCustom->setEnabled(true);
        if (m_btnBrowseCustomPic) m_btnBrowseCustomPic->setEnabled(isCustom);
        if (m_rbPicStretch) m_rbPicStretch->setEnabled(true);
        if (m_rbPicCrop) m_rbPicCrop->setEnabled(true);

        if (m_rbPicAlignTop) m_rbPicAlignTop->setEnabled(false);
        if (m_rbPicAlignBottom) m_rbPicAlignBottom->setEnabled(false);
        if (m_chkSidebarPicExtendEdges) m_chkSidebarPicExtendEdges->setEnabled(false);
    } else { // Picture
        if (m_rbPicEmbedded) m_rbPicEmbedded->setEnabled(true);
        if (m_cmbPicEmbedded) m_cmbPicEmbedded->setEnabled(!isCustom);
        if (m_rbPicCustom) m_rbPicCustom->setEnabled(true);
        if (m_btnBrowseCustomPic) m_btnBrowseCustomPic->setEnabled(isCustom);
        if (m_rbPicStretch) m_rbPicStretch->setEnabled(true);
        if (m_rbPicCrop) m_rbPicCrop->setEnabled(true);

        if (m_rbPicAlignTop) m_rbPicAlignTop->setEnabled(true);
        if (m_rbPicAlignBottom) m_rbPicAlignBottom->setEnabled(true);
        if (m_chkSidebarPicExtendEdges) m_chkSidebarPicExtendEdges->setEnabled(true);
    }
}

void ClassicXSettingsDialog::onSidebarPicModeChanged(int)
{
    updateSidebarPictureUI();
}

void ClassicXSettingsDialog::onSidebarPicSourceToggled(bool)
{
    updateSidebarPictureUI();
}

void ClassicXSettingsDialog::onSidebarPicColorizeToggled(bool enabled)
{
    bool picActive = m_chkShowSidebar && m_chkShowSidebar->isChecked() &&
                     m_cmbSidebarPicMode && m_cmbSidebarPicMode->currentItem() != 0;
    if (m_btnSidebarPicColor)
        m_btnSidebarPicColor->setEnabled(picActive && enabled);
}

void ClassicXSettingsDialog::onSidebarPicColorClicked()
{
    if (!m_chkSidebarPicColorize || !m_chkSidebarPicColorize->isChecked())
        return;
    TQColor col = TQColorDialog::getColor(m_sidebarPicColor, this);
    if (col.isValid()) {
        m_sidebarPicColor = col;
        updateColorButton(m_btnSidebarPicColor, m_sidebarPicColor);
        onSettingsEdited();
    }
}

void ClassicXSettingsDialog::onBrowseCustomPicClicked()
{
    TQString fn = KFileDialog::getOpenFileName(m_customPicPath, "image/png image/jpeg image/x-xpixmap", this, "Select Custom Picture");
    if (!fn.isEmpty()) {
        m_customPicPath = fn;
        onSettingsEdited();
    }
}

void ClassicXSettingsDialog::onShowSidebarToggled(bool enabled)
{
    m_spinWidth->setEnabled(enabled);
    m_chkSidebarHover->setEnabled(enabled);
    bool hoverEnabled = m_chkSidebarHover->isChecked();
    m_lblSidebarHoverDelay->setEnabled(enabled && hoverEnabled);
    m_spinSidebarHoverDelay->setEnabled(enabled && hoverEnabled);

    if (m_lblSidebarButtonsAlign) m_lblSidebarButtonsAlign->setEnabled(enabled);
    if (m_cmbSidebarButtonsAlign) m_cmbSidebarButtonsAlign->setEnabled(enabled);

    if (m_lblButtonsHeader) m_lblButtonsHeader->setEnabled(enabled);
    if (m_chkSidebarUserMenu) m_chkSidebarUserMenu->setEnabled(enabled);
    if (m_chkSidebarShutdownMenu) m_chkSidebarShutdownMenu->setEnabled(enabled);
    if (m_chkSidebarSettings) m_chkSidebarSettings->setEnabled(enabled);
    if (m_chkSidebarDocuments) m_chkSidebarDocuments->setEnabled(enabled);
    if (m_chkSidebarImages) m_chkSidebarImages->setEnabled(enabled);

    if (m_lblUserShutdownHeight) m_lblUserShutdownHeight->setEnabled(enabled);
    if (m_rbUserShutdownFullHeight) m_rbUserShutdownFullHeight->setEnabled(enabled);
    if (m_rbUserShutdownCustomHeight) m_rbUserShutdownCustomHeight->setEnabled(enabled);
    if (m_spinUserShutdownCustomHeight) {
        bool customActive = m_rbUserShutdownCustomHeight && m_rbUserShutdownCustomHeight->isChecked();
        m_spinUserShutdownCustomHeight->setEnabled(enabled && customActive);
    }

    updateSidebarPictureUI();
    updateSpecialItemsEnableState();
}

void ClassicXSettingsDialog::onSidebarHoverToggled(bool enabled)
{
    bool sideEnabled = m_chkShowSidebar->isChecked();
    m_lblSidebarHoverDelay->setEnabled(sideEnabled && enabled);
    m_spinSidebarHoverDelay->setEnabled(sideEnabled && enabled);
}

void ClassicXSettingsDialog::updateShownAppsCountEnabled()
{
    const bool on = m_chkShowRecentApps && m_chkShowRecentApps->isChecked();
    if (m_cmbRecentMode)
        m_cmbRecentMode->setEnabled(on);
    if (m_lblShownApps)
        m_lblShownApps->setEnabled(on);
    if (m_spinNumRecentApps)
        m_spinNumRecentApps->setEnabled(on);
}

void ClassicXSettingsDialog::onShowRecentAppsToggled(bool)
{
    updateShownAppsCountEnabled();
}

void ClassicXSettingsDialog::onShowRecentDocsToggled(bool enabled)
{
    if (m_lblMaxRecentDocs) m_lblMaxRecentDocs->setEnabled(enabled);
    if (m_spinMaxRecentDocs) m_spinMaxRecentDocs->setEnabled(enabled);
}

void ClassicXSettingsDialog::onShowAppIconsToggled(bool enabled)
{
    if (m_lblTreeIconSize)
        m_lblTreeIconSize->setEnabled(enabled);
    if (m_cmbTreeIconSize)
        m_cmbTreeIconSize->setEnabled(enabled);
}

void ClassicXSettingsDialog::onOpacitySliderChanged(int val)
{
    m_lblOpacityVal->setText(TQString::number(val) + "%");
}

void ClassicXSettingsDialog::onOkClicked()
{
    ClassicXSettings::setShowControlCenter(m_chkControlCenter->isChecked());
    ClassicXSettings::setShowRunCommand(m_chkShowRunCommand->isChecked());
    ClassicXSettings::setAnimateOpening(m_chkAnimateOpening->isChecked());
    ClassicXSettings::setAlwaysShowSearchBar(m_chkAlwaysShowSearchBar->isChecked());

    ClassicXSettings::setShowRecentApps(m_chkShowRecentApps->isChecked());
    ClassicXSettings::setRecentVsOften(m_cmbRecentMode->currentItem() == 0);
    ClassicXSettings::setNumRecentApps(m_spinNumRecentApps->value());
    ClassicXSettings::setNumMostUsedApps(m_spinNumRecentApps->value());
    ClassicXSettings::setMaxSearchResults(m_spinMaxSearchResults->value());

    int selectedFmt = m_cmbMenuEntryFormat->currentItem();
    TQString fmtStr = "DescriptionAndName";
    if (selectedFmt == 0) fmtStr = "NameOnly";
    else if (selectedFmt == 1) fmtStr = "NameAndDescription";
    else if (selectedFmt == 2) fmtStr = "DescriptionOnly";
    else fmtStr = "DescriptionAndName";

    // Write via TDESharedConfig (matching KControl / tdebase)
    TDESharedConfig::Ptr kickerSharedCfg = TDESharedConfig::openConfig("kickerrc");
    kickerSharedCfg->reparseConfiguration();
    kickerSharedCfg->setGroup("menus");
    kickerSharedCfg->writeEntry("MenuEntryFormat", fmtStr);

    TQStringList extList = kickerSharedCfg->readListEntry("Extensions");

    struct ExtHelper {
        static void update(TQStringList &list, const TQString &name, bool active) {
            if (active) {
                if (!list.contains(name)) list.append(name);
            } else {
                list.remove(name);
            }
        }
    };

    ExtHelper::update(extList, "printmenu.desktop", m_chkShowPrintSystem->isChecked());
    ExtHelper::update(extList, "remotemenu.desktop", m_chkShowNetworkFolders->isChecked());
    ExtHelper::update(extList, "systemmenu.desktop", m_chkShowSystemMenu->isChecked());
    ExtHelper::update(extList, "recentdocs.desktop", m_chkShowRecentDocs->isChecked());

    kickerSharedCfg->writeEntry("Extensions", extList);
    kickerSharedCfg->writeEntry("UseBookmarks", m_chkShowBookmarks->isChecked());
    kickerSharedCfg->writeEntry("UseBrowser", m_chkShowQuickBrowser->isChecked());
    kickerSharedCfg->sync();
    kickerSharedCfg->reparseConfiguration();

    TDESharedConfig::Ptr kdeGlobalsCfg = TDESharedConfig::openConfig("kdeglobals");
    kdeGlobalsCfg->setGroup("RecentDocuments");
    kdeGlobalsCfg->writeEntry("MaxEntries", m_spinMaxRecentDocs->value());
    kdeGlobalsCfg->sync();
    kdeGlobalsCfg->reparseConfiguration();

    ClassicXSettings::setUseBookmarks(m_chkShowBookmarks->isChecked());
    ClassicXSettings::setUseBrowser(m_chkShowQuickBrowser->isChecked());

    // Also update ClassicXSettings skeleton for local applet
    ClassicXSettings::setMenuEntryFormat(selectedFmt);
    ClassicXSettings::setShowAppIcons(m_chkShowAppIcons->isChecked());
    if (m_cmbTreeIconSize) {
        ClassicXSettings::setMenuEntryHeight(
            treeIconComboIndexToHeight(m_cmbTreeIconSize->currentItem()));
    }

    // DCOP notify kicker (matching KControl / tdebase)
    if (kapp && kapp->dcopClient()) {
        if (!kapp->dcopClient()->isAttached()) {
            kapp->dcopClient()->attach();
        }
        TQByteArray data;
        kapp->dcopClient()->send("kicker", "kicker", "configure()", data);
    }

    int opacityVal = 100 - m_sliderOpacity->value();
    ClassicXSettings::setClassicKMenuOpacity(opacityVal);
    ClassicXSettings::setUseSidePixmap(m_chkShowSidebar->isChecked());
    ClassicXSettings::setSideBarWidth(m_spinWidth->value());
    ClassicXSettings::setSidebarHoverMenu(m_chkSidebarHover->isChecked());
    ClassicXSettings::setSidebarHoverDelay(m_spinSidebarHoverDelay->value());
    ClassicXSettings::setSidebarButtonsAlign(m_cmbSidebarButtonsAlign->currentItem());

    ClassicXSettings::setFullUserShutdownMenuHeight(m_rbUserShutdownFullHeight->isChecked());
    ClassicXSettings::setCustomUserShutdownMenuHeight(m_spinUserShutdownCustomHeight->value());

    ClassicXSettings::setShowSpecialUserMenu(m_chkShowSpecialUserMenu->isChecked());
    ClassicXSettings::setShowSpecialShutdownMenu(m_chkShowSpecialShutdownMenu->isChecked());

    ClassicXSettings::setShowSidebarUserMenu(m_chkSidebarUserMenu->isChecked());
    ClassicXSettings::setShowSidebarShutdownMenu(m_chkSidebarShutdownMenu->isChecked());
    ClassicXSettings::setShowSidebarSettings(m_chkSidebarSettings->isChecked());
    ClassicXSettings::setShowSidebarDocuments(m_chkSidebarDocuments->isChecked());
    ClassicXSettings::setShowSidebarImages(m_chkSidebarImages->isChecked());

    ClassicXSettings::setShowShutdownPowerOff(m_chkShutdownPowerOff->isChecked());
    ClassicXSettings::setShowShutdownReboot(m_chkShutdownReboot->isChecked());
    ClassicXSettings::setShowShutdownSuspend(m_chkShutdownSuspend->isChecked());
    ClassicXSettings::setShowShutdownHybridSuspend(m_chkShutdownHybridSuspend->isChecked());
    ClassicXSettings::setShowShutdownHibernate(m_chkShutdownHibernate->isChecked());

    int iconType = 0;
    if (m_rbIconTDE->isChecked()) iconType = 1;
    else if (m_rbIconCustom->isChecked()) iconType = 2;

    ClassicXSettings::setIconType(iconType);
    ClassicXSettings::setEmbeddedIcon(m_cmbEmbeddedIcon->currentText());
    ClassicXSettings::setCustomIconPath(m_customIconPath);
    ClassicXSettings::setFullScaleStartIcon(m_chkFullScaleStartIcon->isChecked());
    ClassicXSettings::setInvertStartIcon(m_chkInvertStartIcon->isChecked());
    ClassicXSettings::setColorizeStartIcon(m_chkColorizeStartIcon->isChecked());
    ClassicXSettings::setStartIconColor(m_startIconColor);
    ClassicXSettings::setInvertUiIcons(m_chkInvertUiIcons->isChecked());
    ClassicXSettings::setColorizeUiIcons(m_chkColorizeUiIcons->isChecked());
    ClassicXSettings::setUiIconColor(m_uiIconColor);

    if (m_cmbUiIconSize) {
        int selectedSize = 20;
        TQString txt = m_cmbUiIconSize->currentText();
        txt.replace(" px", "");
        bool ok = false;
        int parsed = txt.toInt(&ok);
        if (ok && parsed >= 20) {
            selectedSize = parsed;
        }
        ClassicXSettings::setUiIconSize(selectedSize);
    }

    ClassicXSettings::writeConfig();

    RecentlyLaunchedApps::the().syncRuntimeWithSettings();

    TDEConfig config("classicxapplet_rc");
    config.setGroup("Sidebar");
    config.writeEntry("SidebarPictureMode", m_cmbSidebarPicMode->currentItem());
    config.writeEntry("SidebarPictureSource", m_rbPicCustom->isChecked() ? 1 : 0);
    config.writeEntry("SidebarPictureEmbedded", m_cmbPicEmbedded->currentText());
    config.writeEntry("SidebarPictureCustomPath", m_customPicPath);
    config.writeEntry("SidebarPictureWidthMode", m_rbPicCrop->isChecked() ? 1 : 0);
    config.writeEntry("SidebarPictureAlignMode", m_rbPicAlignBottom->isChecked() ? 1 : 0);
    config.writeEntry("SidebarPictureExtendEdges", m_chkSidebarPicExtendEdges->isChecked());
    config.writeEntry("SidebarPictureColorize", m_chkSidebarPicColorize->isChecked());
    config.writeEntry("SidebarPictureColor", m_sidebarPicColor.name());
    ClassicXSettings::setSidebarPictureExtendEdges(m_chkSidebarPicExtendEdges->isChecked());

    config.setGroup("Colors");
    config.writeEntry("ColorMode", m_cmbColorMode->currentItem());
    config.writeEntry("FgColor", m_fgCustom.name());
    config.writeEntry("BgColor", m_bgCustom.name());
    config.writeEntry("SidebarBgColor", m_sidebarBgCustom.isValid() ? m_sidebarBgCustom.name() : "");
    config.writeEntry("TextBgColor", m_textBgCustom.name());
    config.writeEntry("TitleFgColor", m_titleFgCustom.name());
    config.writeEntry("TitleBgColor", m_titleBgCustom.name());
    config.writeEntry("SearchTextColor", m_searchTextCustom.isValid() ? m_searchTextCustom.name() : "");
    config.writeEntry("ButtonHoverColor", m_buttonHoverBgCustom.isValid() ? m_buttonHoverBgCustom.name() : "");

    config.setGroup("KMenu");
    config.writeEntry("TopPicMode", m_cmbTopPicMode->currentItem());
    config.writeEntry("TopPicEmbedded", m_cmbTopPicEmbedded->currentText());
    config.writeEntry("TopPicCustomLeft", m_topPicLeftPath);
    config.writeEntry("TopPicCustomCenter", m_topPicCenterPath);
    config.writeEntry("TopPicCustomRight", m_topPicRightPath);
    config.writeEntry("TopPicColorize", m_chkTopPicColorize->isChecked());
    config.writeEntry("TopPicColor", m_topPicColor.name());
    config.writeEntry("TopPicShowText", m_chkTopPicShowText->isChecked());
    config.writeEntry("TopPicShowUser", m_chkTopPicUseUser->isChecked());
    config.writeEntry("TopPicShowCustomText", m_chkTopPicUseCustom->isChecked());
    config.writeEntry("TopPicText", m_editTopPicText->text());
    config.writeEntry("TopPicTextColorMode", m_cmbTopPicTextColorMode->currentItem());
    config.writeEntry("TopPicTextColor", m_topPicTextColor.isValid() ? m_topPicTextColor.name() : "");
    config.writeEntry("TopPicShowDate", m_chkTopPicUseDate->isChecked());
    config.writeEntry("TopPicShowTime", m_chkTopPicUseTime->isChecked());

    ClassicXSettings::setTopPicMode(m_cmbTopPicMode->currentItem());
    ClassicXSettings::setTopPicEmbedded(m_cmbTopPicEmbedded->currentText());
    ClassicXSettings::setTopPicCustomLeft(m_topPicLeftPath);
    ClassicXSettings::setTopPicCustomCenter(m_topPicCenterPath);
    ClassicXSettings::setTopPicCustomRight(m_topPicRightPath);
    ClassicXSettings::setTopPicColorize(m_chkTopPicColorize->isChecked());
    ClassicXSettings::setTopPicColor(m_topPicColor);
    ClassicXSettings::setTopPicShowText(m_chkTopPicShowText->isChecked());
    ClassicXSettings::setTopPicShowUser(m_chkTopPicUseUser->isChecked());
    ClassicXSettings::setTopPicShowCustomText(m_chkTopPicUseCustom->isChecked());
    ClassicXSettings::setTopPicText(m_editTopPicText->text());
    ClassicXSettings::setTopPicTextColorMode(m_cmbTopPicTextColorMode->currentItem());
    ClassicXSettings::setTopPicTextColor(m_topPicTextColor);
    ClassicXSettings::setTopPicShowDate(m_chkTopPicUseDate->isChecked());
    ClassicXSettings::setTopPicShowTime(m_chkTopPicUseTime->isChecked());

    config.setGroup("Font");
    config.writeEntry("FontMode", m_cmbFontMode->currentItem());
    config.writeEntry("Font", m_customFont.toString());

    config.setGroup("Button");
    config.writeEntry("IconType", iconType);
    config.writeEntry("EmbeddedIcon", m_cmbEmbeddedIcon->currentText());
    config.writeEntry("CustomIconPath", m_customIconPath);
    config.writeEntry("FullScaleStartIcon", m_chkFullScaleStartIcon->isChecked());
    config.writeEntry("InvertStartIcon", m_chkInvertStartIcon->isChecked());
    config.writeEntry("ColorizeStartIcon", m_chkColorizeStartIcon->isChecked());
    config.writeEntry("StartIconColor", m_startIconColor.name());

    config.setGroup("UIIcons");
    config.writeEntry("InvertUiIcons", m_chkInvertUiIcons->isChecked());
    config.writeEntry("ColorizeUiIcons", m_chkColorizeUiIcons->isChecked());
    config.writeEntry("UiIconColor", m_uiIconColor.name());

    struct IconConfigKey { const char* typeKey; const char* pathKey; };
    IconConfigKey keys[9] = {
        {"ShutdownIconType", "ShutdownCustomIconPath"},
        {"StandbyIconType", "StandbyCustomIconPath"},
        {"LogoutIconType", "LogoutCustomIconPath"},
        {"RestartIconType", "RestartCustomIconPath"},
        {"HibernateIconType", "HibernateCustomIconPath"},
        {"HybridSleepIconType", "HybridSleepCustomIconPath"},
        {"DocumentsIconType", "DocumentsCustomIconPath"},
        {"ImagesIconType", "ImagesCustomIconPath"},
        {"SettingsIconType", "SettingsCustomIconPath"}
    };

    ClassicXSettings::setShutdownIconType(uiIconStoredType(m_uiIconControls[0].cmbSource->currentItem()));
    ClassicXSettings::setShutdownCustomIconPath(m_uiIconControls[0].customPath);

    ClassicXSettings::setStandbyIconType(uiIconStoredType(m_uiIconControls[1].cmbSource->currentItem()));
    ClassicXSettings::setStandbyCustomIconPath(m_uiIconControls[1].customPath);

    ClassicXSettings::setLogoutIconType(uiIconStoredType(m_uiIconControls[2].cmbSource->currentItem()));
    ClassicXSettings::setLogoutCustomIconPath(m_uiIconControls[2].customPath);

    ClassicXSettings::setRestartIconType(uiIconStoredType(m_uiIconControls[3].cmbSource->currentItem()));
    ClassicXSettings::setRestartCustomIconPath(m_uiIconControls[3].customPath);

    ClassicXSettings::setHibernateIconType(uiIconStoredType(m_uiIconControls[4].cmbSource->currentItem()));
    ClassicXSettings::setHibernateCustomIconPath(m_uiIconControls[4].customPath);

    ClassicXSettings::setHybridSleepIconType(uiIconStoredType(m_uiIconControls[5].cmbSource->currentItem()));
    ClassicXSettings::setHybridSleepCustomIconPath(m_uiIconControls[5].customPath);

    ClassicXSettings::setDocumentsIconType(uiIconStoredType(m_uiIconControls[6].cmbSource->currentItem()));
    ClassicXSettings::setDocumentsCustomIconPath(m_uiIconControls[6].customPath);

    ClassicXSettings::setImagesIconType(uiIconStoredType(m_uiIconControls[7].cmbSource->currentItem()));
    ClassicXSettings::setImagesCustomIconPath(m_uiIconControls[7].customPath);

    ClassicXSettings::setSettingsIconType(uiIconStoredType(m_uiIconControls[8].cmbSource->currentItem()));
    ClassicXSettings::setSettingsCustomIconPath(m_uiIconControls[8].customPath);

    for (int i = 0; i < UI_ICON_COUNT; ++i) {
        int t = uiIconStoredType(m_uiIconControls[i].cmbSource->currentItem());
        config.writeEntry(keys[i].typeKey, t);
        config.writeEntry(keys[i].pathKey, m_uiIconControls[i].customPath);
    }
    config.sync();

    TDEConfig kickerConfig("kickerrc");
    kickerConfig.setGroup("Menus");
    TQColor bgVal = (m_cmbColorMode->currentItem() == 0) ? TQColor(245, 246, 248) :
                    ((m_cmbColorMode->currentItem() == 1) ? TQApplication::palette().active().background() : m_bgCustom);
    kickerConfig.writeEntry("ClassicKMenuBackgroundColor", bgVal);
    kickerConfig.sync();

    // Clear caches once all settings are committed
    KickerLib::clearColorCache();
    KickerLib::clearMenuIconSetCache();
    EmbeddedIcons::clearCache();
    ClassicX::clearWindowOpacityCache();

    PanelKMenu* menu = dynamic_cast<PanelKMenu*>(parent());
    if (menu) {
        KickerLib::updateMenuPalette(menu);
        ClassicXButton* btn = dynamic_cast<ClassicXButton*>(menu->appletButton());
        if (btn) {
            ClassicXApplet* applet = dynamic_cast<ClassicXApplet*>(btn->parent());
            if (applet) {
                applet->updateIcon();
            } else {
                int panelH = btn->height() > 0 ? btn->height() : 32;
                TQPixmap px = EmbeddedIcons::loadStartMenuIcon(panelH);
                btn->setIconPixmap(px);
                btn->configure();
                btn->update();
            }
        }
    }

    accept();
}

void ClassicXSettingsDialog::onUserShutdownHeightToggled(bool enabled)
{
    bool sideEnabled = m_chkShowSidebar && m_chkShowSidebar->isChecked();
    if (m_spinUserShutdownCustomHeight) {
        m_spinUserShutdownCustomHeight->setEnabled(sideEnabled && !enabled);
    }
}

void ClassicXSettingsDialog::onIconTypeChanged()
{
    updateIconPreview();
}

void ClassicXSettingsDialog::onEmbeddedIconChanged(int)
{
    updateIconPreview();
}

void ClassicXSettingsDialog::onBrowseCustomIconClicked()
{
    TQString selected = TDEIconDialog::getIcon(TDEIcon::Panel, TDEIcon::Application, false, 0, false, this, i18n("Select Menu Icon"));
    if (!selected.isEmpty()) {
        m_customIconPath = selected;
        updateIconPreview();
        onSettingsEdited();
    }
}

void ClassicXSettingsDialog::onInvertStartIconToggled(bool)
{
    updateIconPreview();
}

void ClassicXSettingsDialog::onColorizeStartIconToggled(bool enabled)
{
    if (m_btnStartIconColor) {
        m_btnStartIconColor->setEnabled(enabled);
    }
    updateIconPreview();
}

void ClassicXSettingsDialog::onStartIconColorClicked()
{
    if (!m_chkColorizeStartIcon || !m_chkColorizeStartIcon->isChecked()) return;

    TQColor col = TQColorDialog::getColor(m_startIconColor, this);
    if (col.isValid()) {
        m_startIconColor = col;
        updateColorButton(m_btnStartIconColor, m_startIconColor);
        updateIconPreview();
        onSettingsEdited();
    }
}

void ClassicXSettingsDialog::onInvertUiIconsToggled(bool)
{
    updateUiIconPreviews();
}

void ClassicXSettingsDialog::onColorizeUiIconsToggled(bool enabled)
{
    if (m_btnUiIconColor) {
        m_btnUiIconColor->setEnabled(enabled);
    }
    updateUiIconPreviews();
}

void ClassicXSettingsDialog::onUiIconColorClicked()
{
    if (!m_chkColorizeUiIcons || !m_chkColorizeUiIcons->isChecked()) return;

    TQColor col = TQColorDialog::getColor(m_uiIconColor, this);
    if (col.isValid()) {
        m_uiIconColor = col;
        updateColorButton(m_btnUiIconColor, m_uiIconColor);
        updateUiIconPreviews();
        onSettingsEdited();
    }
}

void ClassicXSettingsDialog::onUiIconTypeChanged()
{
    syncUiIconPresetCombo();
    updateUiIconPreviews();
}

void ClassicXSettingsDialog::syncUiIconPresetCombo()
{
    if (!m_cmbUiIconPreset)
        return;

    int first = -1;
    bool mixed = false;
    for (int i = 0; i < UI_ICON_COUNT; ++i) {
        if (!m_uiIconControls[i].cmbSource)
            continue;
        const int v = m_uiIconControls[i].cmbSource->currentItem();
        if (first < 0)
            first = v;
        else if (v != first) {
            mixed = true;
            break;
        }
    }

    m_cmbUiIconPreset->blockSignals(true);
    // Preset only covers Win (0) and KDE (1). All-Custom or mixed → blank.
    if (mixed || first < 0 || first > 1)
        m_cmbUiIconPreset->setCurrentItem(0);
    else
        m_cmbUiIconPreset->setCurrentItem(first + 1);
    m_cmbUiIconPreset->blockSignals(false);
}

void ClassicXSettingsDialog::onUiIconPresetChanged(int index)
{
    if (index <= 0) {
        syncUiIconPresetCombo();
        return;
    }

    const int source = index - 1;
    for (int i = 0; i < UI_ICON_COUNT; ++i) {
        if (!m_uiIconControls[i].cmbSource)
            continue;
        m_uiIconControls[i].cmbSource->blockSignals(true);
        m_uiIconControls[i].cmbSource->setCurrentItem(source);
        m_uiIconControls[i].cmbSource->blockSignals(false);
    }
    updateUiIconPreviews();
}

void ClassicXSettingsDialog::onBrowseUiIconClicked()
{
    const TQObject *snd = sender();
    if (!snd) return;

    for (int i = 0; i < UI_ICON_COUNT; ++i) {
        if (m_uiIconControls[i].btnBrowse == snd) {
            TQString selected = TDEIconDialog::getIcon(TDEIcon::Panel, TDEIcon::Application, false, 0, false, this, i18n("Select Icon"));
            if (!selected.isEmpty()) {
                m_uiIconControls[i].customPath = selected;
                updateUiIconPreviews();
                onSettingsEdited();
            }
            break;
        }
    }
}

void ClassicXSettingsDialog::updateUiIconPreviews()
{
    bool invert = m_chkInvertUiIcons && m_chkInvertUiIcons->isChecked();
    bool colorize = m_chkColorizeUiIcons && m_chkColorizeUiIcons->isChecked();

    for (int i = 0; i < UI_ICON_COUNT; ++i) {
        if (!m_uiIconControls[i].previewLabel) continue;

        TQPixmap px;
        const int combo = m_uiIconControls[i].cmbSource
                              ? m_uiIconControls[i].cmbSource->currentItem()
                              : 0;
        const bool isCustom = (combo == 2);
        if (m_uiIconControls[i].btnBrowse)
            m_uiIconControls[i].btnBrowse->setEnabled(isCustom);

        if (combo == 1) {
            px = uiIconPreviewPixmap(
                TQString::fromLatin1("kde_") + m_uiIconControls[i].embeddedName, 24);
            if (px.isNull())
                px = uiIconPreviewPixmap(m_uiIconControls[i].embeddedName, 24);
        } else if (isCustom) {
            if (!m_uiIconControls[i].customPath.isEmpty()) {
                if (TQFile::exists(m_uiIconControls[i].customPath)) {
                    px = TQPixmap(m_uiIconControls[i].customPath);
                    if (!px.isNull() && (px.width() != 24 || px.height() != 24)) {
                        px.convertFromImage(TQImage(px.convertToImage()).smoothScale(24, 24));
                    }
                } else {
                    px = TDEGlobal::iconLoader()->loadIcon(m_uiIconControls[i].customPath, TDEIcon::Small, 24);
                }
            }
            if (px.isNull())
                px = uiIconPreviewPixmap(m_uiIconControls[i].embeddedName, 24);
        } else {
            px = uiIconPreviewPixmap(m_uiIconControls[i].embeddedName, 24);
        }

        if (!px.isNull() && (invert || (colorize && m_uiIconColor.isValid()))) {
            TQImage img = px.convertToImage();
            if (invert) {
                EmbeddedIcons::invertImage(img);
            }
            if (colorize && m_uiIconColor.isValid()) {
                EmbeddedIcons::colorizeImage(img, m_uiIconColor, invert);
            }
            px.convertFromImage(img);
        }

        if (px.isNull()) {
            m_uiIconControls[i].previewLabel->setPixmap(TQPixmap());
            m_uiIconControls[i].previewLabel->setText("No Icon");
        } else {
            m_uiIconControls[i].previewLabel->setPixmap(px);
            m_uiIconControls[i].previewLabel->setText(TQString::null);
        }
    }
}

void ClassicXSettingsDialog::updateIconPreview()
{
    TQPixmap px;
    int targetH = 32;

    if (m_rbIconEmbedded->isChecked()) {
        m_cmbEmbeddedIcon->setEnabled(true);
        m_btnBrowseCustomIcon->setEnabled(false);
        TQString name = m_cmbEmbeddedIcon->currentText();
        TQImage img = EmbeddedIcons::getNativeImage(name);
        if (!img.isNull() && img.height() > 0) {
            int newW = KMAX(1, (int)((double)img.width() * ((double)targetH / (double)img.height())));
            px.convertFromImage(img.smoothScale(newW, targetH));
        }
    }
    else if (m_rbIconTDE->isChecked()) {
        m_cmbEmbeddedIcon->setEnabled(false);
        m_btnBrowseCustomIcon->setEnabled(false);
        TDEConfig kickerrc("kickerrc");
        kickerrc.setGroup("KMenu");
        TQString custIcon = kickerrc.readEntry("CustomIcon", "");
        TQImage img;
        if (!custIcon.isEmpty() && TQFile::exists(custIcon)) {
            TQPixmap tempPx(custIcon);
            if (!tempPx.isNull()) img = tempPx.convertToImage();
        }
        if (img.isNull()) {
            TQPixmap sysPx = TDEGlobal::iconLoader()->loadIcon("kmenu", TDEIcon::Panel, targetH);
            if (!sysPx.isNull()) img = sysPx.convertToImage();
        }
        if (!img.isNull() && img.height() > 0) {
            int newW = KMAX(1, (int)((double)img.width() * ((double)targetH / (double)img.height())));
            px.convertFromImage(img.smoothScale(newW, targetH));
        }
    }
    else if (m_rbIconCustom->isChecked()) {
        m_cmbEmbeddedIcon->setEnabled(false);
        m_btnBrowseCustomIcon->setEnabled(true);
        TQImage img;
        if (!m_customIconPath.isEmpty()) {
            if (TQFile::exists(m_customIconPath)) {
                TQPixmap tempPx(m_customIconPath);
                if (!tempPx.isNull()) img = tempPx.convertToImage();
            } else {
                TQPixmap sysPx = TDEGlobal::iconLoader()->loadIcon(m_customIconPath, TDEIcon::Panel, targetH);
                if (!sysPx.isNull()) img = sysPx.convertToImage();
            }
        }
        if (!img.isNull() && img.height() > 0) {
            int newW = KMAX(1, (int)((double)img.width() * ((double)targetH / (double)img.height())));
            px.convertFromImage(img.smoothScale(newW, targetH));
        }
    }

    bool invert = m_chkInvertStartIcon && m_chkInvertStartIcon->isChecked();
    bool colorize = m_chkColorizeStartIcon && m_chkColorizeStartIcon->isChecked();

    if (!px.isNull() && (invert || (colorize && m_startIconColor.isValid()))) {
        TQImage img = px.convertToImage();
        if (invert) {
            EmbeddedIcons::invertImage(img);
        }
        if (colorize && m_startIconColor.isValid()) {
            EmbeddedIcons::colorizeImage(img, m_startIconColor, invert);
        }
        px.convertFromImage(img);
    }

    if (px.isNull()) {
        m_lblStartIconPreview->setFixedSize(80, 38);
        m_lblStartIconPreview->setPixmap(TQPixmap());
        m_lblStartIconPreview->setText("No Icon");
    } else {
        int frameW = px.width() + 8;
        if (frameW < 38) frameW = 38;
        m_lblStartIconPreview->setFixedSize(frameW, 38);
        m_lblStartIconPreview->setPixmap(px);
        m_lblStartIconPreview->setText(TQString::null);
    }
}

void ClassicXSettingsDialog::onSidebarWidthChanged(int width)
{
    updateUiIconSizeCombo(width);
}

bool ClassicXSettingsDialog::sidebarHasButtons() const
{
    return (m_chkSidebarUserMenu && m_chkSidebarUserMenu->isChecked())
        || (m_chkSidebarShutdownMenu && m_chkSidebarShutdownMenu->isChecked())
        || (m_chkSidebarSettings && m_chkSidebarSettings->isChecked())
        || (m_chkSidebarDocuments && m_chkSidebarDocuments->isChecked())
        || (m_chkSidebarImages && m_chkSidebarImages->isChecked());
}

void ClassicXSettingsDialog::updateSidebarWidthConstraints()
{
    if (m_updatingSidebarConstraints || !m_spinWidth)
        return;

    m_updatingSidebarConstraints = true;

    const int absMin = 16;
    const int absMax = 80;
    int minW = absMin;
    if (sidebarHasButtons()) {
        minW = currentUiIconSize() + 2;
        if (minW < absMin)
            minW = absMin;
        if (minW > absMax)
            minW = absMax;
    }

    const int oldVal = m_spinWidth->value();
    m_spinWidth->setMinValue(minW);
    if (m_spinWidth->value() < minW)
        m_spinWidth->setValue(minW);

    updateUiIconSizeCombo(m_spinWidth->value());

    m_updatingSidebarConstraints = false;

    if (oldVal != m_spinWidth->value())
        onSettingsEdited();
}

void ClassicXSettingsDialog::updateUiIconSizeCombo(int sidebarWidth)
{
    if (!m_cmbUiIconSize) return;

    int currentSel = ClassicXSettings::uiIconSize();
    if (m_cmbUiIconSize->count() > 0) {
        TQString txt = m_cmbUiIconSize->currentText();
        txt.replace(" px", "");
        bool ok = false;
        int parsed = txt.toInt(&ok);
        if (ok && parsed >= 20) currentSel = parsed;
    }

    int maxAllowed = 60;
    if (sidebarHasButtons()) {
        maxAllowed = sidebarWidth - 2;
        if (maxAllowed > 60) maxAllowed = 60;
        if (maxAllowed < 20) maxAllowed = 20;
    }

    m_cmbUiIconSize->clear();
    int selectIdx = 0;
    int idx = 0;

    for (int s = 20; s <= maxAllowed; s += 2) {
        m_cmbUiIconSize->insertItem(TQString("%1 px").arg(s));
        if (s == currentSel || (currentSel > s && currentSel < s + 2)) {
            selectIdx = idx;
        }
        idx++;
    }

    if (currentSel > maxAllowed) {
        selectIdx = m_cmbUiIconSize->count() - 1;
    }

    if (m_cmbUiIconSize->count() > 0) {
        m_cmbUiIconSize->setCurrentItem(selectIdx);
    }
}

void ClassicXSettingsDialog::onCancelClicked()
{
    reject();
}

void ClassicXSettingsDialog::onEditMenuClicked()
{
    TDEApplication::startServiceByDesktopName("kmenuedit", TQStringList());
    reject();
}

void ClassicXSettingsDialog::onAboutClicked()
{
    const TQColor bgCol(0x68, 0x68, 0x64);
    const TQColor titleCol(0xFF, 0xFF, 0xFF);
    const TQColor creditCol(0xEE, 0xEE, 0xEE);

    TQDialog about(this, "ClassicXAbout", true);
    about.setCaption(i18n("About ClassicX"));
    about.setFixedSize(480, 350);
    about.setPaletteBackgroundColor(bgCol);
    about.setPaletteForegroundColor(titleCol);

    TQVBoxLayout *lay = new TQVBoxLayout(&about, 16, 8);

    TQLabel *iconLbl = new TQLabel(&about);
    iconLbl->setAlignment(TQt::AlignHCenter);
    iconLbl->setPaletteBackgroundColor(bgCol);
    TQImage img = EmbeddedIcons::getNativeImage("about");
    if (!img.isNull()) {
        TQPixmap px;
        px.convertFromImage(img.smoothScale(150, 150));
        iconLbl->setPixmap(px);
        iconLbl->setFixedSize(150, 150);
    }
    lay->addWidget(iconLbl, 0, TQt::AlignHCenter);
    lay->addSpacing(8);

    TQLabel *title = new TQLabel("ClassicX", &about);
    TQFont titleFont = title->font();
    titleFont.setBold(true);
    if (titleFont.pointSize() > 0)
        titleFont.setPointSize(titleFont.pointSize() + 6);
    else
        titleFont.setPixelSize(titleFont.pixelSize() + 8);
    title->setFont(titleFont);
    title->setAlignment(TQt::AlignHCenter);
    title->setPaletteBackgroundColor(bgCol);
    title->setPaletteForegroundColor(titleCol);
    lay->addWidget(title);

    TQLabel *desc = new TQLabel("A start Menu for Trinity Desktop", &about);
    TQFont descFont = desc->font();
    if (descFont.pointSize() > 0)
        descFont.setPointSize(descFont.pointSize() + 2);
    else
        descFont.setPixelSize(descFont.pixelSize() + 3);
    desc->setFont(descFont);
    desc->setAlignment(TQt::AlignHCenter);
    desc->setPaletteBackgroundColor(bgCol);
    desc->setPaletteForegroundColor(titleCol);
    lay->addWidget(desc);

    TQLabel *by = new TQLabel("by seb3737 - https://github.com/seb3773", &about);
    TQFont byFont = by->font();
    byFont.setItalic(true);
    by->setFont(byFont);
    by->setAlignment(TQt::AlignHCenter);
    by->setPaletteBackgroundColor(bgCol);
    by->setPaletteForegroundColor(creditCol);
    lay->addWidget(by);

    lay->addStretch(1);

    TQPushButton *ok = new TQPushButton("OK", &about);
    ok->setDefault(true);
    ok->setFocus();
    connect(ok, TQT_SIGNAL(clicked()), &about, TQT_SLOT(accept()));
    TQHBoxLayout *okRow = new TQHBoxLayout();
    okRow->addStretch(1);
    okRow->addWidget(ok);
    okRow->addStretch(1);
    lay->addLayout(okRow);

    about.exec();
}

int ClassicXSettingsDialog::currentUiIconSize() const
{
    if (!m_cmbUiIconSize || m_cmbUiIconSize->count() == 0)
        return ClassicXSettings::uiIconSize();
    TQString txt = m_cmbUiIconSize->currentText();
    txt.replace(" px", "");
    bool ok = false;
    const int parsed = txt.toInt(&ok);
    if (ok && parsed >= 20)
        return parsed;
    return ClassicXSettings::uiIconSize();
}

void ClassicXSettingsDialog::captureDialogState(TQMap<TQString, TQString>& m) const
{
    m.clear();

    if (m_cmbMenuEntryFormat) profilePutNum(m, "MenuEntryFormat", m_cmbMenuEntryFormat->currentItem());
    if (m_chkShowAppIcons) profilePutBool(m, "ShowAppIcons", m_chkShowAppIcons->isChecked());
    if (m_cmbTreeIconSize) profilePutNum(m, "TreeIconSize", m_cmbTreeIconSize->currentItem());
    if (m_chkAnimateOpening) profilePutBool(m, "AnimateOpening", m_chkAnimateOpening->isChecked());
    if (m_chkAlwaysShowSearchBar) profilePutBool(m, "AlwaysShowSearchBar", m_chkAlwaysShowSearchBar->isChecked());

    if (m_chkShowRunCommand) profilePutBool(m, "ShowRunCommand", m_chkShowRunCommand->isChecked());
    if (m_chkControlCenter) profilePutBool(m, "ShowControlCenter", m_chkControlCenter->isChecked());
    if (m_chkShowBookmarks) profilePutBool(m, "UseBookmarks", m_chkShowBookmarks->isChecked());
    if (m_chkShowPrintSystem) profilePutBool(m, "ShowPrintSystem", m_chkShowPrintSystem->isChecked());
    if (m_chkShowQuickBrowser) profilePutBool(m, "UseBrowser", m_chkShowQuickBrowser->isChecked());
    if (m_chkShowNetworkFolders) profilePutBool(m, "ShowNetworkFolders", m_chkShowNetworkFolders->isChecked());
    if (m_chkShowSystemMenu) profilePutBool(m, "ShowSystemMenu", m_chkShowSystemMenu->isChecked());
    if (m_chkShowRecentDocs) profilePutBool(m, "ShowRecentDocs", m_chkShowRecentDocs->isChecked());
    if (m_chkShowSpecialUserMenu) profilePutBool(m, "ShowSpecialUserMenu", m_chkShowSpecialUserMenu->isChecked());
    if (m_chkShowSpecialShutdownMenu) profilePutBool(m, "ShowSpecialShutdownMenu", m_chkShowSpecialShutdownMenu->isChecked());

    if (m_chkShowRecentApps) profilePutBool(m, "ShowRecentApps", m_chkShowRecentApps->isChecked());
    if (m_cmbRecentMode) profilePutNum(m, "RecentMode", m_cmbRecentMode->currentItem());
    if (m_spinNumRecentApps) profilePutNum(m, "NumRecentApps", m_spinNumRecentApps->value());
    if (m_spinMaxRecentDocs) profilePutNum(m, "MaxRecentDocs", m_spinMaxRecentDocs->value());
    if (m_spinMaxSearchResults) profilePutNum(m, "MaxSearchResults", m_spinMaxSearchResults->value());
    if (m_sliderOpacity) profilePutNum(m, "Transparency", m_sliderOpacity->value());

    if (m_cmbColorMode) profilePutNum(m, "ColorMode", m_cmbColorMode->currentItem());
    profilePutColor(m, "FgColor", m_fgCustom);
    profilePutColor(m, "BgColor", m_bgCustom);
    profilePutColor(m, "SidebarBgColor", m_sidebarBgCustom);
    profilePutColor(m, "TextBgColor", m_textBgCustom);
    profilePutColor(m, "TitleFgColor", m_titleFgCustom);
    profilePutColor(m, "TitleBgColor", m_titleBgCustom);
    profilePutColor(m, "SearchTextColor", m_searchTextCustom);
    profilePutColor(m, "ButtonHoverColor", m_buttonHoverBgCustom);

    if (m_cmbFontMode) {
        int fm = m_cmbFontMode->currentItem();
        profilePutNum(m, "FontMode", fm);
        if (fm != 0)
            profilePut(m, "Font", m_customFont.toString());
    }

    if (m_chkShowSidebar) profilePutBool(m, "UseSidePixmap", m_chkShowSidebar->isChecked());
    if (m_spinWidth) profilePutNum(m, "SideBarWidth", m_spinWidth->value());
    if (m_chkSidebarHover) profilePutBool(m, "SidebarHover", m_chkSidebarHover->isChecked());
    if (m_spinSidebarHoverDelay) profilePutNum(m, "SidebarHoverDelay", m_spinSidebarHoverDelay->value());
    if (m_cmbSidebarButtonsAlign) profilePutNum(m, "SidebarButtonsAlign", m_cmbSidebarButtonsAlign->currentItem());
    if (m_chkSidebarUserMenu) profilePutBool(m, "ShowSidebarUserMenu", m_chkSidebarUserMenu->isChecked());
    if (m_chkSidebarShutdownMenu) profilePutBool(m, "ShowSidebarShutdownMenu", m_chkSidebarShutdownMenu->isChecked());
    if (m_chkSidebarSettings) profilePutBool(m, "ShowSidebarSettings", m_chkSidebarSettings->isChecked());
    if (m_chkSidebarDocuments) profilePutBool(m, "ShowSidebarDocuments", m_chkSidebarDocuments->isChecked());
    if (m_chkSidebarImages) profilePutBool(m, "ShowSidebarImages", m_chkSidebarImages->isChecked());
    if (m_rbUserShutdownFullHeight) profilePutBool(m, "FullUserShutdownHeight", m_rbUserShutdownFullHeight->isChecked());
    if (m_spinUserShutdownCustomHeight) profilePutNum(m, "CustomUserShutdownHeight", m_spinUserShutdownCustomHeight->value());

    if (m_chkShutdownPowerOff) profilePutBool(m, "ShutdownPowerOff", m_chkShutdownPowerOff->isChecked());
    if (m_chkShutdownReboot) profilePutBool(m, "ShutdownReboot", m_chkShutdownReboot->isChecked());
    if (m_chkShutdownSuspend) profilePutBool(m, "ShutdownSuspend", m_chkShutdownSuspend->isChecked());
    if (m_chkShutdownHybridSuspend) profilePutBool(m, "ShutdownHybridSuspend", m_chkShutdownHybridSuspend->isChecked());
    if (m_chkShutdownHibernate) profilePutBool(m, "ShutdownHibernate", m_chkShutdownHibernate->isChecked());

    int iconType = 0;
    if (m_rbIconTDE && m_rbIconTDE->isChecked()) iconType = 1;
    else if (m_rbIconCustom && m_rbIconCustom->isChecked()) iconType = 2;
    profilePutNum(m, "IconType", iconType);
    if (m_cmbEmbeddedIcon) profilePut(m, "EmbeddedIcon", m_cmbEmbeddedIcon->currentText());
    profilePut(m, "CustomIconPath", m_customIconPath);
    if (m_chkFullScaleStartIcon) profilePutBool(m, "FullScaleStartIcon", m_chkFullScaleStartIcon->isChecked());
    if (m_chkInvertStartIcon) profilePutBool(m, "InvertStartIcon", m_chkInvertStartIcon->isChecked());
    if (m_chkColorizeStartIcon) profilePutBool(m, "ColorizeStartIcon", m_chkColorizeStartIcon->isChecked());
    profilePutColor(m, "StartIconColor", m_startIconColor);

    if (m_chkInvertUiIcons) profilePutBool(m, "InvertUiIcons", m_chkInvertUiIcons->isChecked());
    if (m_chkColorizeUiIcons) profilePutBool(m, "ColorizeUiIcons", m_chkColorizeUiIcons->isChecked());
    profilePutColor(m, "UiIconColor", m_uiIconColor);
    profilePutNum(m, "UiIconSize", currentUiIconSize());
    for (int i = 0; i < UI_ICON_COUNT; ++i) {
        const TQString typeKey = TQString("UiIconSource%1").arg(i);
        const TQString pathKey = TQString("UiIconPath%1").arg(i);
        if (m_uiIconControls[i].cmbSource)
            m.insert(typeKey, TQString::number(m_uiIconControls[i].cmbSource->currentItem()));
        m.insert(pathKey, m_uiIconControls[i].customPath);
    }

    if (m_cmbSidebarPicMode) profilePutNum(m, "SidebarPictureMode", m_cmbSidebarPicMode->currentItem());
    if (m_rbPicCustom) profilePutNum(m, "SidebarPictureSource", m_rbPicCustom->isChecked() ? 1 : 0);
    if (m_cmbPicEmbedded) profilePut(m, "SidebarPictureEmbedded", m_cmbPicEmbedded->currentText());
    profilePut(m, "SidebarPictureCustomPath", m_customPicPath);
    if (m_rbPicCrop) profilePutNum(m, "SidebarPictureWidthMode", m_rbPicCrop->isChecked() ? 1 : 0);
    if (m_rbPicAlignBottom) profilePutNum(m, "SidebarPictureAlignMode", m_rbPicAlignBottom->isChecked() ? 1 : 0);
    if (m_chkSidebarPicExtendEdges) profilePutBool(m, "SidebarPictureExtendEdges", m_chkSidebarPicExtendEdges->isChecked());
    if (m_chkSidebarPicColorize) profilePutBool(m, "SidebarPictureColorize", m_chkSidebarPicColorize->isChecked());
    profilePutColor(m, "SidebarPictureColor", m_sidebarPicColor);

    if (m_cmbTopPicMode) profilePutNum(m, "TopPicMode", m_cmbTopPicMode->currentItem());
    if (m_cmbTopPicEmbedded) profilePut(m, "TopPicEmbedded", m_cmbTopPicEmbedded->currentText());
    profilePut(m, "TopPicCustomLeft", m_topPicLeftPath);
    profilePut(m, "TopPicCustomCenter", m_topPicCenterPath);
    profilePut(m, "TopPicCustomRight", m_topPicRightPath);
    if (m_chkTopPicColorize) profilePutBool(m, "TopPicColorize", m_chkTopPicColorize->isChecked());
    profilePutColor(m, "TopPicColor", m_topPicColor);
    if (m_chkTopPicShowText) profilePutBool(m, "TopPicShowText", m_chkTopPicShowText->isChecked());
    if (m_chkTopPicUseUser) profilePutBool(m, "TopPicShowUser", m_chkTopPicUseUser->isChecked());
    if (m_chkTopPicUseCustom) profilePutBool(m, "TopPicShowCustomText", m_chkTopPicUseCustom->isChecked());
    if (m_editTopPicText) profilePut(m, "TopPicText", m_editTopPicText->text());
    if (m_cmbTopPicTextColorMode) profilePutNum(m, "TopPicTextColorMode", m_cmbTopPicTextColorMode->currentItem());
    profilePutColor(m, "TopPicTextColor", m_topPicTextColor);
    if (m_chkTopPicUseDate) profilePutBool(m, "TopPicShowDate", m_chkTopPicUseDate->isChecked());
    if (m_chkTopPicUseTime) profilePutBool(m, "TopPicShowTime", m_chkTopPicUseTime->isChecked());
}

void ClassicXSettingsDialog::applyDialogState(const TQMap<TQString, TQString>& m)
{
    m_ignoreProfileDirty = true;
    if (m_spinWidth)
        m_spinWidth->setMinValue(16);

    if (profileHas(m, "MenuEntryFormat")) setComboIndex(m_cmbMenuEntryFormat, profileGetInt(m, "MenuEntryFormat"));
    if (profileHas(m, "ShowAppIcons") && m_chkShowAppIcons) m_chkShowAppIcons->setChecked(profileGetBool(m, "ShowAppIcons"));
    if (profileHas(m, "TreeIconSize")) setComboIndex(m_cmbTreeIconSize, profileGetInt(m, "TreeIconSize"));
    if (profileHas(m, "AnimateOpening") && m_chkAnimateOpening)
        m_chkAnimateOpening->setChecked(profileGetBool(m, "AnimateOpening"));
    if (profileHas(m, "AlwaysShowSearchBar") && m_chkAlwaysShowSearchBar)
        m_chkAlwaysShowSearchBar->setChecked(profileGetBool(m, "AlwaysShowSearchBar"));

    if (profileHas(m, "ShowRunCommand") && m_chkShowRunCommand) m_chkShowRunCommand->setChecked(profileGetBool(m, "ShowRunCommand"));
    if (profileHas(m, "ShowControlCenter") && m_chkControlCenter) m_chkControlCenter->setChecked(profileGetBool(m, "ShowControlCenter"));
    if (profileHas(m, "UseBookmarks") && m_chkShowBookmarks) m_chkShowBookmarks->setChecked(profileGetBool(m, "UseBookmarks"));
    if (profileHas(m, "ShowPrintSystem") && m_chkShowPrintSystem) m_chkShowPrintSystem->setChecked(profileGetBool(m, "ShowPrintSystem"));
    if (profileHas(m, "UseBrowser") && m_chkShowQuickBrowser) m_chkShowQuickBrowser->setChecked(profileGetBool(m, "UseBrowser"));
    if (profileHas(m, "ShowNetworkFolders") && m_chkShowNetworkFolders) m_chkShowNetworkFolders->setChecked(profileGetBool(m, "ShowNetworkFolders"));
    if (profileHas(m, "ShowSystemMenu") && m_chkShowSystemMenu) m_chkShowSystemMenu->setChecked(profileGetBool(m, "ShowSystemMenu"));
    if (profileHas(m, "ShowRecentDocs") && m_chkShowRecentDocs) m_chkShowRecentDocs->setChecked(profileGetBool(m, "ShowRecentDocs"));
    if (profileHas(m, "ShowSpecialUserMenu") && m_chkShowSpecialUserMenu) m_chkShowSpecialUserMenu->setChecked(profileGetBool(m, "ShowSpecialUserMenu"));
    if (profileHas(m, "ShowSpecialShutdownMenu") && m_chkShowSpecialShutdownMenu)
        m_chkShowSpecialShutdownMenu->setChecked(profileGetBool(m, "ShowSpecialShutdownMenu"));

    if (profileHas(m, "ShowRecentApps") && m_chkShowRecentApps) m_chkShowRecentApps->setChecked(profileGetBool(m, "ShowRecentApps"));
    if (profileHas(m, "RecentMode")) setComboIndex(m_cmbRecentMode, profileGetInt(m, "RecentMode"));
    if (profileHas(m, "NumRecentApps") && m_spinNumRecentApps) m_spinNumRecentApps->setValue(profileGetInt(m, "NumRecentApps"));
    if (profileHas(m, "MaxRecentDocs") && m_spinMaxRecentDocs) m_spinMaxRecentDocs->setValue(profileGetInt(m, "MaxRecentDocs"));
    if (profileHas(m, "MaxSearchResults") && m_spinMaxSearchResults) m_spinMaxSearchResults->setValue(profileGetInt(m, "MaxSearchResults"));
    if (profileHas(m, "Transparency") && m_sliderOpacity) m_sliderOpacity->setValue(profileGetInt(m, "Transparency"));

    if (profileHas(m, "ColorMode")) setComboIndex(m_cmbColorMode, profileGetInt(m, "ColorMode"));
    if (profileHas(m, "FgColor")) m_fgCustom = profileGetColor(m, "FgColor");
    if (profileHas(m, "BgColor")) m_bgCustom = profileGetColor(m, "BgColor");
    if (profileHas(m, "SidebarBgColor")) m_sidebarBgCustom = profileGetColor(m, "SidebarBgColor");
    if (profileHas(m, "TextBgColor")) m_textBgCustom = profileGetColor(m, "TextBgColor");
    if (profileHas(m, "TitleFgColor")) m_titleFgCustom = profileGetColor(m, "TitleFgColor");
    if (profileHas(m, "TitleBgColor")) m_titleBgCustom = profileGetColor(m, "TitleBgColor");
    if (profileHas(m, "SearchTextColor")) m_searchTextCustom = profileGetColor(m, "SearchTextColor");
    if (profileHas(m, "ButtonHoverColor")) m_buttonHoverBgCustom = profileGetColor(m, "ButtonHoverColor");

    if (profileHas(m, "FontMode")) setComboIndex(m_cmbFontMode, profileGetInt(m, "FontMode"));
    if (profileHas(m, "Font")) {
        TQFont f;
        if (f.fromString(profileGet(m, "Font")))
            m_customFont = f;
    }

    if (profileHas(m, "UseSidePixmap") && m_chkShowSidebar) m_chkShowSidebar->setChecked(profileGetBool(m, "UseSidePixmap"));
    if (profileHas(m, "SideBarWidth") && m_spinWidth) m_spinWidth->setValue(profileGetInt(m, "SideBarWidth"));
    if (profileHas(m, "SidebarHover") && m_chkSidebarHover) m_chkSidebarHover->setChecked(profileGetBool(m, "SidebarHover"));
    if (profileHas(m, "SidebarHoverDelay") && m_spinSidebarHoverDelay) m_spinSidebarHoverDelay->setValue(profileGetInt(m, "SidebarHoverDelay"));
    if (profileHas(m, "SidebarButtonsAlign")) setComboIndex(m_cmbSidebarButtonsAlign, profileGetInt(m, "SidebarButtonsAlign"));
    if (profileHas(m, "ShowSidebarUserMenu") && m_chkSidebarUserMenu) m_chkSidebarUserMenu->setChecked(profileGetBool(m, "ShowSidebarUserMenu"));
    if (profileHas(m, "ShowSidebarShutdownMenu") && m_chkSidebarShutdownMenu) m_chkSidebarShutdownMenu->setChecked(profileGetBool(m, "ShowSidebarShutdownMenu"));
    if (profileHas(m, "ShowSidebarSettings") && m_chkSidebarSettings) m_chkSidebarSettings->setChecked(profileGetBool(m, "ShowSidebarSettings"));
    if (profileHas(m, "ShowSidebarDocuments") && m_chkSidebarDocuments) m_chkSidebarDocuments->setChecked(profileGetBool(m, "ShowSidebarDocuments"));
    if (profileHas(m, "ShowSidebarImages") && m_chkSidebarImages) m_chkSidebarImages->setChecked(profileGetBool(m, "ShowSidebarImages"));
    if (profileHas(m, "FullUserShutdownHeight") && m_rbUserShutdownFullHeight && m_rbUserShutdownCustomHeight) {
        const bool full = profileGetBool(m, "FullUserShutdownHeight");
        m_rbUserShutdownFullHeight->setChecked(full);
        m_rbUserShutdownCustomHeight->setChecked(!full);
    }
    if (profileHas(m, "CustomUserShutdownHeight") && m_spinUserShutdownCustomHeight)
        m_spinUserShutdownCustomHeight->setValue(profileGetInt(m, "CustomUserShutdownHeight"));

    if (profileHas(m, "ShutdownPowerOff") && m_chkShutdownPowerOff) m_chkShutdownPowerOff->setChecked(profileGetBool(m, "ShutdownPowerOff"));
    if (profileHas(m, "ShutdownReboot") && m_chkShutdownReboot) m_chkShutdownReboot->setChecked(profileGetBool(m, "ShutdownReboot"));
    if (profileHas(m, "ShutdownSuspend") && m_chkShutdownSuspend) m_chkShutdownSuspend->setChecked(profileGetBool(m, "ShutdownSuspend"));
    if (profileHas(m, "ShutdownHybridSuspend") && m_chkShutdownHybridSuspend) m_chkShutdownHybridSuspend->setChecked(profileGetBool(m, "ShutdownHybridSuspend"));
    if (profileHas(m, "ShutdownHibernate") && m_chkShutdownHibernate) m_chkShutdownHibernate->setChecked(profileGetBool(m, "ShutdownHibernate"));

    if (profileHas(m, "IconType")) {
        const int iconType = profileGetInt(m, "IconType");
        if (iconType == 1 && m_rbIconTDE) m_rbIconTDE->setChecked(true);
        else if (iconType == 2 && m_rbIconCustom) m_rbIconCustom->setChecked(true);
        else if (m_rbIconEmbedded) m_rbIconEmbedded->setChecked(true);
    }
    if (profileHas(m, "EmbeddedIcon")) setComboByText(m_cmbEmbeddedIcon, profileGet(m, "EmbeddedIcon"));
    if (profileHas(m, "CustomIconPath")) m_customIconPath = profileGet(m, "CustomIconPath");
    if (profileHas(m, "FullScaleStartIcon") && m_chkFullScaleStartIcon) m_chkFullScaleStartIcon->setChecked(profileGetBool(m, "FullScaleStartIcon"));
    if (profileHas(m, "InvertStartIcon") && m_chkInvertStartIcon) m_chkInvertStartIcon->setChecked(profileGetBool(m, "InvertStartIcon"));
    if (profileHas(m, "ColorizeStartIcon") && m_chkColorizeStartIcon) m_chkColorizeStartIcon->setChecked(profileGetBool(m, "ColorizeStartIcon"));
    if (profileHas(m, "StartIconColor")) m_startIconColor = profileGetColor(m, "StartIconColor");

    if (profileHas(m, "InvertUiIcons") && m_chkInvertUiIcons) m_chkInvertUiIcons->setChecked(profileGetBool(m, "InvertUiIcons"));
    if (profileHas(m, "ColorizeUiIcons") && m_chkColorizeUiIcons) m_chkColorizeUiIcons->setChecked(profileGetBool(m, "ColorizeUiIcons"));
    if (profileHas(m, "UiIconColor")) m_uiIconColor = profileGetColor(m, "UiIconColor");
    for (int i = 0; i < UI_ICON_COUNT; ++i) {
        const TQString typeKey = TQString("UiIconSource%1").arg(i);
        const TQString pathKey = TQString("UiIconPath%1").arg(i);
        TQMap<TQString, TQString>::ConstIterator typeIt = m.find(typeKey);
        if (typeIt != m.end() && m_uiIconControls[i].cmbSource)
            setComboIndex(m_uiIconControls[i].cmbSource, typeIt.data().toInt());
        TQMap<TQString, TQString>::ConstIterator pathIt = m.find(pathKey);
        if (pathIt != m.end())
            m_uiIconControls[i].customPath = pathIt.data();
    }

    if (profileHas(m, "SidebarPictureMode")) setComboIndex(m_cmbSidebarPicMode, profileGetInt(m, "SidebarPictureMode"));
    if (profileHas(m, "SidebarPictureSource") && m_rbPicEmbedded && m_rbPicCustom) {
        const bool custom = profileGetInt(m, "SidebarPictureSource") != 0;
        m_rbPicCustom->setChecked(custom);
        m_rbPicEmbedded->setChecked(!custom);
    }
    if (profileHas(m, "SidebarPictureCustomPath")) m_customPicPath = profileGet(m, "SidebarPictureCustomPath");
    if (profileHas(m, "SidebarPictureWidthMode") && m_rbPicStretch && m_rbPicCrop) {
        const bool crop = profileGetInt(m, "SidebarPictureWidthMode") != 0;
        m_rbPicCrop->setChecked(crop);
        m_rbPicStretch->setChecked(!crop);
    }
    if (profileHas(m, "SidebarPictureAlignMode") && m_rbPicAlignTop && m_rbPicAlignBottom) {
        const bool bottom = profileGetInt(m, "SidebarPictureAlignMode") != 0;
        m_rbPicAlignBottom->setChecked(bottom);
        m_rbPicAlignTop->setChecked(!bottom);
    }
    if (profileHas(m, "SidebarPictureExtendEdges") && m_chkSidebarPicExtendEdges)
        m_chkSidebarPicExtendEdges->setChecked(profileGetBool(m, "SidebarPictureExtendEdges"));
    if (profileHas(m, "SidebarPictureColorize") && m_chkSidebarPicColorize)
        m_chkSidebarPicColorize->setChecked(profileGetBool(m, "SidebarPictureColorize"));
    if (profileHas(m, "SidebarPictureColor")) m_sidebarPicColor = profileGetColor(m, "SidebarPictureColor");

    if (profileHas(m, "TopPicMode")) setComboIndex(m_cmbTopPicMode, profileGetInt(m, "TopPicMode"));
    if (profileHas(m, "TopPicEmbedded")) setComboByText(m_cmbTopPicEmbedded, profileGet(m, "TopPicEmbedded"));
    if (profileHas(m, "TopPicCustomLeft")) {
        m_topPicLeftPath = profileGet(m, "TopPicCustomLeft");
        if (m_lblTopPicLeftPath)
            m_lblTopPicLeftPath->setText(m_topPicLeftPath.isEmpty() ? TQString::fromLatin1("None") : m_topPicLeftPath.section('/', -1));
    }
    if (profileHas(m, "TopPicCustomCenter")) {
        m_topPicCenterPath = profileGet(m, "TopPicCustomCenter");
        if (m_lblTopPicCenterPath)
            m_lblTopPicCenterPath->setText(m_topPicCenterPath.isEmpty() ? TQString::fromLatin1("None") : m_topPicCenterPath.section('/', -1));
    }
    if (profileHas(m, "TopPicCustomRight")) {
        m_topPicRightPath = profileGet(m, "TopPicCustomRight");
        if (m_lblTopPicRightPath)
            m_lblTopPicRightPath->setText(m_topPicRightPath.isEmpty() ? TQString::fromLatin1("None") : m_topPicRightPath.section('/', -1));
    }
    if (profileHas(m, "TopPicColorize") && m_chkTopPicColorize) m_chkTopPicColorize->setChecked(profileGetBool(m, "TopPicColorize"));
    if (profileHas(m, "TopPicColor")) m_topPicColor = profileGetColor(m, "TopPicColor");

    if (m_chkTopPicUseUser) m_chkTopPicUseUser->blockSignals(true);
    if (m_chkTopPicUseCustom) m_chkTopPicUseCustom->blockSignals(true);
    if (m_chkTopPicUseDate) m_chkTopPicUseDate->blockSignals(true);
    if (m_chkTopPicUseTime) m_chkTopPicUseTime->blockSignals(true);
    if (m_chkTopPicShowText) m_chkTopPicShowText->blockSignals(true);
    if (profileHas(m, "TopPicShowText") && m_chkTopPicShowText) m_chkTopPicShowText->setChecked(profileGetBool(m, "TopPicShowText"));
    if (profileHas(m, "TopPicShowUser") && m_chkTopPicUseUser) m_chkTopPicUseUser->setChecked(profileGetBool(m, "TopPicShowUser"));
    if (profileHas(m, "TopPicShowCustomText") && m_chkTopPicUseCustom) m_chkTopPicUseCustom->setChecked(profileGetBool(m, "TopPicShowCustomText"));
    if (profileHas(m, "TopPicShowDate") && m_chkTopPicUseDate) m_chkTopPicUseDate->setChecked(profileGetBool(m, "TopPicShowDate"));
    if (profileHas(m, "TopPicShowTime") && m_chkTopPicUseTime) m_chkTopPicUseTime->setChecked(profileGetBool(m, "TopPicShowTime"));
    if (m_chkTopPicUseUser) m_chkTopPicUseUser->blockSignals(false);
    if (m_chkTopPicUseCustom) m_chkTopPicUseCustom->blockSignals(false);
    if (m_chkTopPicUseDate) m_chkTopPicUseDate->blockSignals(false);
    if (m_chkTopPicUseTime) m_chkTopPicUseTime->blockSignals(false);
    if (m_chkTopPicShowText) m_chkTopPicShowText->blockSignals(false);

    if (profileHas(m, "TopPicText") && m_editTopPicText) m_editTopPicText->setText(profileGet(m, "TopPicText"));
    if (profileHas(m, "TopPicTextColorMode")) setComboIndex(m_cmbTopPicTextColorMode, profileGetInt(m, "TopPicTextColorMode"));
    if (profileHas(m, "TopPicTextColor")) m_topPicTextColor = profileGetColor(m, "TopPicTextColor");

    onShowAppIconsToggled(m_chkShowAppIcons && m_chkShowAppIcons->isChecked());
    updateShownAppsCountEnabled();
    onShowRecentDocsToggled(m_chkShowRecentDocs && m_chkShowRecentDocs->isChecked());
    if (m_chkShowSidebar)
        onShowSidebarToggled(m_chkShowSidebar->isChecked());
    if (m_chkSidebarHover)
        onSidebarHoverToggled(m_chkSidebarHover->isChecked());
    onUserShutdownHeightToggled(m_rbUserShutdownFullHeight && m_rbUserShutdownFullHeight->isChecked());
    refreshColorButtons();
    updateFontUI();
    updateSidebarPictureUI();
    if (profileHas(m, "SidebarPictureEmbedded"))
        setComboByText(m_cmbPicEmbedded, profileGet(m, "SidebarPictureEmbedded"));
    updateTopPictureUI();
    if (m_btnStartIconColor)
        updateColorButton(m_btnStartIconColor, m_startIconColor);
    if (m_btnStartIconColor && m_chkColorizeStartIcon)
        m_btnStartIconColor->setEnabled(m_chkColorizeStartIcon->isChecked());
    if (m_btnUiIconColor)
        updateColorButton(m_btnUiIconColor, m_uiIconColor);
    if (m_btnSidebarPicColor)
        updateColorButton(m_btnSidebarPicColor, m_sidebarPicColor);
    if (m_btnTopPicColor)
        updateColorButton(m_btnTopPicColor, m_topPicColor);
    onTopPicTextColorModeChanged(m_cmbTopPicTextColorMode ? m_cmbTopPicTextColorMode->currentItem() : 0);
    updateIconPreview();
    updateUiIconPreviews();
    syncUiIconPresetCombo();
    updateSpecialItemsEnableState();

    if (profileHas(m, "UiIconSize") && m_cmbUiIconSize) {
        const int want = profileGetInt(m, "UiIconSize");
        for (int i = 0; i < m_cmbUiIconSize->count(); ++i) {
            TQString txt = m_cmbUiIconSize->text(i);
            txt.replace(" px", "");
            bool ok = false;
            if (txt.toInt(&ok) == want && ok) {
                m_cmbUiIconSize->setCurrentItem(i);
                break;
            }
        }
    }

    updateSidebarWidthConstraints();

    m_ignoreProfileDirty = false;
}

bool ClassicXSettingsDialog::dialogStateEquals(const TQMap<TQString, TQString>& m) const
{
    TQMap<TQString, TQString> current;
    captureDialogState(current);
    if (current.isEmpty() || m.isEmpty())
        return false;

    TQMap<TQString, TQString>::ConstIterator it = current.begin();
    for (; it != current.end(); ++it) {
        TQMap<TQString, TQString>::ConstIterator fit = m.find(it.key());
        if (fit == m.end() || fit.data() != it.data())
            return false;
    }
    return true;
}

void ClassicXSettingsDialog::writeDialogToConfig(TDEConfig& cfg) const
{
    TQMap<TQString, TQString> m;
    captureDialogState(m);
    cfg.setGroup("Dialog");
    TQMap<TQString, TQString>::ConstIterator it = m.begin();
    for (; it != m.end(); ++it)
        cfg.writeEntry(it.key(), it.data());
}

void ClassicXSettingsDialog::applyConfigToDialog(TDEConfig& cfg)
{
    applyDialogState(cfg.entryMap("Dialog"));
}

bool ClassicXSettingsDialog::dialogMatchesConfig(TDEConfig& cfg) const
{
    return dialogStateEquals(cfg.entryMap("Dialog"));
}

void ClassicXSettingsDialog::refreshProfileList()
{
    TQString keep;
    if (m_cmbProfile && m_cmbProfile->currentItem() > 0)
        keep = m_cmbProfile->currentText();

    m_profilePaths.clear();
    m_profilePaths.append(TQString::null);

    if (m_cmbProfile) {
        m_cmbProfile->blockSignals(true);
        m_cmbProfile->clear();
        m_cmbProfile->insertItem(TQString::fromLatin1(""));
    }

    const TQValueList<ClassicXProfileInfo> list = ClassicXProfile::list();
    int select = 0;
    for (TQValueList<ClassicXProfileInfo>::ConstIterator it = list.begin(); it != list.end(); ++it) {
        if (m_cmbProfile)
            m_cmbProfile->insertItem((*it).name);
        m_profilePaths.append((*it).filePath);
        if (!keep.isEmpty() && (*it).name == keep)
            select = (int)m_profilePaths.count() - 1;
    }

    if (m_cmbProfile) {
        m_cmbProfile->setCurrentItem(select);
        m_cmbProfile->blockSignals(false);
    }
}

void ClassicXSettingsDialog::selectMatchingProfile()
{
    if (!m_cmbProfile)
        return;
    for (unsigned int i = 1; i < m_profilePaths.count(); ++i) {
        TQMap<TQString, TQString> state;
        if (ClassicXProfile::loadState(m_profilePaths[i], state)) {
            if (dialogStateEquals(state)) {
                m_cmbProfile->blockSignals(true);
                m_cmbProfile->setCurrentItem((int)i);
                m_cmbProfile->blockSignals(false);
                return;
            }
        }
    }
}

void ClassicXSettingsDialog::updateProfileButtons()
{
    const bool has = m_cmbProfile && m_cmbProfile->currentItem() > 0;
    if (m_btnProfileSave)
        m_btnProfileSave->setEnabled(!has);
    bool canDelete = has;
    if (has && m_cmbProfile->currentItem() < (int)m_profilePaths.count())
        canDelete = ClassicXProfile::isUserFile(m_profilePaths[m_cmbProfile->currentItem()]);
    if (m_btnProfileDelete)
        m_btnProfileDelete->setEnabled(canDelete);
}

void ClassicXSettingsDialog::connectProfileDirtyTracking()
{
    connectProfileDirty(m_chkShowAppIcons, TQT_SIGNAL(toggled(bool)), this);
    connectProfileDirty(m_chkAnimateOpening, TQT_SIGNAL(toggled(bool)), this);
    connectProfileDirty(m_chkAlwaysShowSearchBar, TQT_SIGNAL(toggled(bool)), this);
    connectProfileDirty(m_chkShowRunCommand, TQT_SIGNAL(toggled(bool)), this);
    connectProfileDirty(m_chkControlCenter, TQT_SIGNAL(toggled(bool)), this);
    connectProfileDirty(m_chkShowBookmarks, TQT_SIGNAL(toggled(bool)), this);
    connectProfileDirty(m_chkShowPrintSystem, TQT_SIGNAL(toggled(bool)), this);
    connectProfileDirty(m_chkShowQuickBrowser, TQT_SIGNAL(toggled(bool)), this);
    connectProfileDirty(m_chkShowNetworkFolders, TQT_SIGNAL(toggled(bool)), this);
    connectProfileDirty(m_chkShowSystemMenu, TQT_SIGNAL(toggled(bool)), this);
    connectProfileDirty(m_chkShowRecentDocs, TQT_SIGNAL(toggled(bool)), this);
    connectProfileDirty(m_chkShowSpecialUserMenu, TQT_SIGNAL(toggled(bool)), this);
    connectProfileDirty(m_chkShowSpecialShutdownMenu, TQT_SIGNAL(toggled(bool)), this);
    connectProfileDirty(m_chkShowRecentApps, TQT_SIGNAL(toggled(bool)), this);
    connectProfileDirty(m_chkShowSidebar, TQT_SIGNAL(toggled(bool)), this);
    connectProfileDirty(m_chkSidebarHover, TQT_SIGNAL(toggled(bool)), this);
    connectProfileDirty(m_chkSidebarUserMenu, TQT_SIGNAL(toggled(bool)), this);
    connectProfileDirty(m_chkSidebarShutdownMenu, TQT_SIGNAL(toggled(bool)), this);
    connectProfileDirty(m_chkSidebarSettings, TQT_SIGNAL(toggled(bool)), this);
    connectProfileDirty(m_chkSidebarDocuments, TQT_SIGNAL(toggled(bool)), this);
    connectProfileDirty(m_chkSidebarImages, TQT_SIGNAL(toggled(bool)), this);
    connectProfileDirty(m_chkShutdownPowerOff, TQT_SIGNAL(toggled(bool)), this);
    connectProfileDirty(m_chkShutdownReboot, TQT_SIGNAL(toggled(bool)), this);
    connectProfileDirty(m_chkShutdownSuspend, TQT_SIGNAL(toggled(bool)), this);
    connectProfileDirty(m_chkShutdownHybridSuspend, TQT_SIGNAL(toggled(bool)), this);
    connectProfileDirty(m_chkShutdownHibernate, TQT_SIGNAL(toggled(bool)), this);
    connectProfileDirty(m_chkFullScaleStartIcon, TQT_SIGNAL(toggled(bool)), this);
    connectProfileDirty(m_chkInvertStartIcon, TQT_SIGNAL(toggled(bool)), this);
    connectProfileDirty(m_chkColorizeStartIcon, TQT_SIGNAL(toggled(bool)), this);
    connectProfileDirty(m_chkInvertUiIcons, TQT_SIGNAL(toggled(bool)), this);
    connectProfileDirty(m_chkColorizeUiIcons, TQT_SIGNAL(toggled(bool)), this);
    connectProfileDirty(m_chkSidebarPicExtendEdges, TQT_SIGNAL(toggled(bool)), this);
    connectProfileDirty(m_chkSidebarPicColorize, TQT_SIGNAL(toggled(bool)), this);
    connectProfileDirty(m_chkTopPicColorize, TQT_SIGNAL(toggled(bool)), this);
    connectProfileDirty(m_chkTopPicShowText, TQT_SIGNAL(toggled(bool)), this);
    connectProfileDirty(m_chkTopPicUseUser, TQT_SIGNAL(toggled(bool)), this);
    connectProfileDirty(m_chkTopPicUseCustom, TQT_SIGNAL(toggled(bool)), this);
    connectProfileDirty(m_chkTopPicUseDate, TQT_SIGNAL(toggled(bool)), this);
    connectProfileDirty(m_chkTopPicUseTime, TQT_SIGNAL(toggled(bool)), this);

    connectProfileDirty(m_rbUserShutdownFullHeight, TQT_SIGNAL(toggled(bool)), this);
    connectProfileDirty(m_rbUserShutdownCustomHeight, TQT_SIGNAL(toggled(bool)), this);
    connectProfileDirty(m_rbPicEmbedded, TQT_SIGNAL(toggled(bool)), this);
    connectProfileDirty(m_rbPicCustom, TQT_SIGNAL(toggled(bool)), this);
    connectProfileDirty(m_rbPicStretch, TQT_SIGNAL(toggled(bool)), this);
    connectProfileDirty(m_rbPicCrop, TQT_SIGNAL(toggled(bool)), this);
    connectProfileDirty(m_rbPicAlignTop, TQT_SIGNAL(toggled(bool)), this);
    connectProfileDirty(m_rbPicAlignBottom, TQT_SIGNAL(toggled(bool)), this);
    connectProfileDirty(m_rbIconEmbedded, TQT_SIGNAL(toggled(bool)), this);
    connectProfileDirty(m_rbIconTDE, TQT_SIGNAL(toggled(bool)), this);
    connectProfileDirty(m_rbIconCustom, TQT_SIGNAL(toggled(bool)), this);

    connectProfileDirty(m_cmbMenuEntryFormat, TQT_SIGNAL(activated(int)), this);
    connectProfileDirty(m_cmbTreeIconSize, TQT_SIGNAL(activated(int)), this);
    connectProfileDirty(m_cmbRecentMode, TQT_SIGNAL(activated(int)), this);
    connectProfileDirty(m_cmbColorMode, TQT_SIGNAL(activated(int)), this);
    connectProfileDirty(m_cmbFontMode, TQT_SIGNAL(activated(int)), this);
    connectProfileDirty(m_cmbSidebarPicMode, TQT_SIGNAL(activated(int)), this);
    connectProfileDirty(m_cmbPicEmbedded, TQT_SIGNAL(activated(int)), this);
    connectProfileDirty(m_cmbTopPicMode, TQT_SIGNAL(activated(int)), this);
    connectProfileDirty(m_cmbTopPicEmbedded, TQT_SIGNAL(activated(int)), this);
    connectProfileDirty(m_cmbTopPicTextColorMode, TQT_SIGNAL(activated(int)), this);
    connectProfileDirty(m_cmbEmbeddedIcon, TQT_SIGNAL(activated(int)), this);
    connectProfileDirty(m_cmbUiIconSize, TQT_SIGNAL(activated(int)), this);
    connectProfileDirty(m_cmbUiIconPreset, TQT_SIGNAL(activated(int)), this);
    connectProfileDirty(m_cmbSidebarButtonsAlign, TQT_SIGNAL(activated(int)), this);
    for (int i = 0; i < UI_ICON_COUNT; ++i)
        connectProfileDirty(m_uiIconControls[i].cmbSource, TQT_SIGNAL(activated(int)), this);

    connectProfileDirty(m_spinNumRecentApps, TQT_SIGNAL(valueChanged(int)), this);
    connectProfileDirty(m_spinMaxRecentDocs, TQT_SIGNAL(valueChanged(int)), this);
    connectProfileDirty(m_spinMaxSearchResults, TQT_SIGNAL(valueChanged(int)), this);
    connectProfileDirty(m_spinWidth, TQT_SIGNAL(valueChanged(int)), this);
    connectProfileDirty(m_spinSidebarHoverDelay, TQT_SIGNAL(valueChanged(int)), this);
    connectProfileDirty(m_spinUserShutdownCustomHeight, TQT_SIGNAL(valueChanged(int)), this);
    connectProfileDirty(m_sliderOpacity, TQT_SIGNAL(valueChanged(int)), this);
    connectProfileDirty(m_editTopPicText, TQT_SIGNAL(textChanged(const TQString&)), this);
}

void ClassicXSettingsDialog::onSettingsEdited()
{
    if (m_ignoreProfileDirty || !m_cmbProfile)
        return;
    if (m_cmbProfile->currentItem() != 0) {
        m_cmbProfile->blockSignals(true);
        m_cmbProfile->setCurrentItem(0);
        m_cmbProfile->blockSignals(false);
    }
    updateProfileButtons();
}

void ClassicXSettingsDialog::onProfileActivated(int index)
{
    if (index <= 0) {
        updateProfileButtons();
        return;
    }
    if (index >= (int)m_profilePaths.count())
        return;
    TQMap<TQString, TQString> state;
    if (ClassicXProfile::loadState(m_profilePaths[index], state)) {
        applyDialogState(state);
    }
    updateProfileButtons();
}

void ClassicXSettingsDialog::onProfileSaveClicked()
{
    bool ok = false;
    const TQString name = TQInputDialog::getText(
        i18n("Save profile"),
        i18n("Profile name:"),
        TQLineEdit::Normal,
        TQString::null,
        &ok,
        this).stripWhiteSpace();
    if (!ok || name.isEmpty())
        return;

    const TQString path = ClassicXProfile::filePathForName(name);
    if (TQFile::exists(path)) {
        if (KMessageBox::warningContinueCancel(
                this,
                i18n("A profile named \"%1\" already exists.\nDo you want to overwrite it?").arg(name),
                i18n("Overwrite Profile"),
                i18n("Overwrite")) != KMessageBox::Continue)
            return;
    }

    KSimpleConfig cfg(path, false);
    cfg.setGroup("Profile");
    cfg.writeEntry("Name", name);
    cfg.writeEntry("Version", 1);
    writeDialogToConfig(cfg);
    cfg.sync();

    refreshProfileList();
    if (m_cmbProfile) {
        m_cmbProfile->blockSignals(true);
        int select = 0;
        for (int i = 1; i < m_cmbProfile->count(); ++i) {
            if (m_cmbProfile->text(i) == name) {
                select = i;
                break;
            }
        }
        m_cmbProfile->setCurrentItem(select);
        m_cmbProfile->blockSignals(false);
    }
    updateProfileButtons();
}

void ClassicXSettingsDialog::onProfileDeleteClicked()
{
    if (!m_cmbProfile || m_cmbProfile->currentItem() <= 0)
        return;
    const int idx = m_cmbProfile->currentItem();
    if (idx >= (int)m_profilePaths.count())
        return;

    const TQString name = m_cmbProfile->currentText();
    const TQString path = m_profilePaths[idx];
    if (!ClassicXProfile::isUserFile(path)) {
        KMessageBox::sorry(this, i18n("This profile cannot be deleted."));
        return;
    }
    if (KMessageBox::warningContinueCancel(
            this,
            i18n("Delete profile \"%1\"?").arg(name),
            i18n("Delete Profile"),
            i18n("Delete")) != KMessageBox::Continue)
        return;

    ClassicXProfile::removeUserFile(path);
    refreshProfileList();
    if (m_cmbProfile) {
        m_cmbProfile->blockSignals(true);
        m_cmbProfile->setCurrentItem(0);
        m_cmbProfile->blockSignals(false);
    }
    updateProfileButtons();
}

#include "classicx_settings_dialog.moc"
