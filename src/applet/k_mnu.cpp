/*****************************************************************

Copyright (c) 1996-2000 the kicker authors. See file AUTHORS.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

******************************************************************/

#include "classicx_applet.h"
#include <dmctl.h>
#include <kipc.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

#include <ksimpleconfig.h>
#include <tdefiledialog.h>
#include <tdeglobalsettings.h>
#include <tqbuttongroup.h>
#include <tqcheckbox.h>
#include <tqcolordialog.h>
#include <tqcursor.h>
#include <tqeventloop.h>
#include <tqfile.h>
#include <tqfontdialog.h>
#include <tqobjectlist.h>
#include <tqframe.h>
#include <tqgroupbox.h>
#include <tqhbox.h>
#include <tqbitmap.h>
#include <tqimage.h>
#include <tqlabel.h>
#include <tqpainter.h>
#include <tqradiobutton.h>
#include <tqslider.h>
#include <tqspinbox.h>
#include <tqstyle.h>
#include <tqtimer.h>
#include <tqtooltip.h>
#include <tqwidgetlist.h>

#include "embedded_icons.h"
#include <dcopclient.h>
#include <kbookmarkmenu.h>
#include <kdebug.h>
#include <kiconloader.h>
#include <klineedit.h>
#include <kstandarddirs.h>
#include <kuser.h>
#include <popupmenutop.h>
#include <tdeaccel.h>
#include <tdeaction.h>
#include <tdeapplication.h>
#include <tdeconfig.h>
#include <tdeglobal.h>
#include <tdelocale.h>
#include <tdemessagebox.h>
#include <tdetoolbarbutton.h>
#include <tqevent.h>
#include <tqlayout.h>
#include <tqregexp.h>

#include "classicxSettings.h"
#include "classicx_settings_dialog.h"
#include "client_mnu.h"
#include "global.h"
#include "menuinfo.h"
#include "menumanager.h"
#include "popupmenutitle.h"
#include "popupmenutop.h"
#include "powermanager_flags.h"
#include "x11opacity.h"

class TopSpacerMenuItem : public TQCustomMenuItem {
public:
  TopSpacerMenuItem(int height) : m_height(height) {}
  virtual bool fullSpan() const { return true; }
  virtual TQSize sizeHint() { return TQSize(0, m_height); }
  virtual void paint(TQPainter *, const TQColorGroup &, bool, bool, int, int,
                     int, int) {
    // Completely invisible spacer item.
  }

private:
  int m_height;
};
#include "classicx_searchlineedit.h"
#include "quickbrowser_mnu.h"
#include "recentapps.h"

#include <dcopref.h>
#include <kbookmarkmanager.h>
#include <kprocess.h>
#include <krun.h>
#include <kstandarddirs.h>
#ifdef WITH_TDEHWLIB
#include <tdehardwaredevices.h>
#endif

#include "k_mnu.h"
#include "k_mnu.moc"
#include <kuser.h>

namespace SuspendType {
enum SuspendType {
  NotSpecified = 0,
  Freeze,
  Standby,
  Suspend,
  Hibernate,
  HybridSuspend
};
};

const int PanelKMenu::searchLineID(23140 /*whatever*/);

namespace {

/** Pad a sidebar popup so its content reaches targetH.
 * Row height follows UiIconSize (same as User/Shutdown items), not font-only.
 * Drop leftover setFixedHeight before measuring, otherwise sizeHint stays
 * stuck at the previous popup geometry until the applet is restarted. */
void padPopupToHeight(TQPopupMenu *menu, const TQIconSet &padIcon,
                      int targetH) {
  if (!menu || targetH <= 0)
    return;

  menu->setMinimumHeight(0);
  menu->setMaximumHeight(32767);
  int keepW = menu->width();
  if (keepW < 100)
    keepW = 100;
  menu->resize(keepW, 1);

  menu->adjustSize();
  int currentH = menu->sizeHint().height();
  if (currentH < 1)
    currentH = menu->height();

  int iconH = ClassicXSettings::uiIconSize();
  if (iconH < 20)
    iconH = 20;
  int rowH = TQFontMetrics(menu->font()).height() + 10;
  if (rowH < iconH + 8)
    rowH = iconH + 8;
  if (rowH < 20)
    rowH = 20;

  int slack = targetH - currentH - 20;
  int numPads = (slack > 0) ? (slack / rowH) : 0;
  if (numPads > 20)
    numPads = 20;

  for (int i = 0; i < numPads; ++i) {
    int id = menu->insertItem(padIcon, TQString(" "), -1, -1);
    menu->setItemEnabled(id, false);
  }
}

} // namespace

PanelKMenu::PanelKMenu()
    : PanelServiceMenu(TQString::null, TQString::null, 0, "KMenu"),
      sessionsMenu(0), logoutMenu(0), treeUserMenu(0), treeShutdownMenu(0),
      client_id(10000), delay_init(false), bookmarkMenu(0), actionCollection(0),
      bookmarkOwner(0), searchEdit(0), blockMouseTimer(0), popupCloseTimer(0),
      m_sidebarPopupHoverTimer(0), m_pendingHoverBtn(-1), m_blockedHoverBtn(-1),
      windowTimerTimedOut(false), m_inFlatSearchMode(false),
      m_treeStashed(false),
      m_servicesCached(false), m_singleMatch(0), m_lastHighlightedId(-1),
      m_hoveredSidebarBtn(-1),
      m_delayedHoverBtn(-1), m_searchBaseHeight(0), m_baseContentFloorWidth(0),
      m_widthBeforeSearch(0), m_canShutdown(false), m_canReboot(false),
      m_canSuspend(false), m_canHibernate(false), m_canHybridSuspend(false),
      m_canFreeze(false), m_powerSystemInitialized(false),
      m_inPopulateSessions(false), m_inPopulateLogout(false),
      m_inEventFilter(false), m_savedRecentCount(0), m_appletButton(0),
      m_currentSessionVt(-1), m_lastTopPicMinute(-1), m_lastSidebarHeight(-1),
      m_lastSidebarWidth(-1), m_lastSidebarIconSize(-1), m_lastSidebarAlign(-1),
      m_lastSidebarUserOnTop(-1),
      m_inOpeningAnim(false), m_openingAnimStep(0), m_totalOpeningAnimSteps(16),
      m_openingAnimDistance(160), m_openingAnimDirection(KPanelApplet::Up),
      m_openingAnimTimer(0), m_hasTopPicMask(false) {
  DCOPObject *prevDCOP = DCOPObject::find("KMenu");
  if (prevDCOP && prevDCOP != static_cast<DCOPObject*>(this)) {
    m_previousKMenuObj = dynamic_cast<TQObject*>(prevDCOP);
  }
  static const TQCString dcopObjId("KMenu");
  DCOPObject::setObjId(dcopObjId);
  // set the first client id to some arbitrarily large value.
  client_id = 10000;
  // Don't automatically clear the main menu.
  disableAutoClear();
  actionCollection = new TDEActionCollection(this);
  setCaption(i18n("TDE Menu"));

  DCOPClient *dcopClient = TDEApplication::dcopClient();
  dcopClient->connectDCOPSignal(
      0, "appLauncher", "serviceStartedByStorageId(TQString,TQString)",
      dcopObjId, "slotServiceStartedByStorageId(TQString,TQString)", false);
  // Legacy KDE3 repair timer - disabled in ClassicX
  // displayRepairTimer = new TQTimer( this );
  // connect( displayRepairTimer, TQT_SIGNAL(timeout()), this,
  // TQT_SLOT(repairDisplay()) );
  blockMouseTimer = new TQTimer(this);
  setMouseTracking(true);

  m_openingAnimTimer = new TQTimer(this);
  connect(m_openingAnimTimer, TQT_SIGNAL(timeout()), this,
          TQT_SLOT(slotOpeningAnimStep()));

  // Initialise power system capabilities asynchronously to avoid startup lag
  TQTimer::singleShot(0, this, TQT_SLOT(slotInitPowerSystem()));

  KickerLib::updateMenuPalette(this);
  ensureLogoutMenu();

  // Grace period timer for sidebar popup closing
  popupCloseTimer = new TQTimer(this);
  connect(popupCloseTimer, TQT_SIGNAL(timeout()),
          TQT_SLOT(slotClosePopupTimeout()));

  // Submenu delay timer for sidebar secondary popups
  m_sidebarPopupHoverTimer = new TQTimer(this);
  connect(m_sidebarPopupHoverTimer, TQT_SIGNAL(timeout()),
          TQT_SLOT(slotSidebarPopupHoverTimeout()));
  m_pendingHoverBtn = -1;
  m_blockedHoverBtn = -1;

  installEventFilter(this);
  kapp->installEventFilter(this);

  // 2b: Connect once here (not gated on search-bar creation).
  // H1 v2: slotOnShow does a cheap recent refresh when already initialized.
  // Runs after PanelServiceMenu's aboutToShow/slotApplyOpacity connections.
  connect(this, TQT_SIGNAL(aboutToShow()), this, TQT_SLOT(slotOnShow()));
  connect(this, TQT_SIGNAL(highlighted(int)), this,
          TQT_SLOT(slotSearchItemHighlighted(int)));
}

PanelKMenu::~PanelKMenu() {
  if (kapp) {
    kapp->removeEventFilter(this);
  }
  DCOPObject::setObjId(TQCString());
  if (m_previousKMenuObj) {
    DCOPObject *prevDCOP = dynamic_cast<DCOPObject*>(m_previousKMenuObj.operator TQObject*());
    if (prevDCOP) {
      prevDCOP->setObjId("KMenu");
    }
  }
  // Tears down dynamic/plugin submenus, bookmarkMenu, and nulls tree* members.
  clearSubmenus();
  delete bookmarkOwner;
  bookmarkOwner = 0;
}

void PanelKMenu::slotServiceStartedByStorageId(TQString starter,
                                               TQString storageId) {
  if (starter != "kmenu") {
    kdDebug() << "KMenu - updating recently used applications: " << storageId
              << endl;
    if (storageId.isEmpty())
      return;
    KService::Ptr service = KService::serviceByStorageId(storageId);
    if (!service)
      return;
    updateRecentlyUsedApps(service);
  }
}

void PanelKMenu::hideMenu() { hide(); }

void PanelKMenu::windowClearTimeout() { windowTimerTimedOut = true; }

bool PanelKMenu::loadSidePixmap() {
  if (!ClassicXSettings::useSidePixmap()) {
    return false;
  }

  int sideWidth = ClassicXSettings::sideBarWidth();
  if (sideWidth < 16)
    sideWidth = 16;
  if (sideWidth > 80)
    sideWidth = 80;
  int sideHeight = 36;

  // Create pixmaps with the background color of the menu
  sidePixmap.resize(sideWidth, sideHeight);
  sideTilePixmap.resize(sideWidth, sideHeight);

  // Get the sidebar background color from the configured setting
  TQColor bgColor = KickerLib::getClassicKMenuSidebarBgColor();

  // Fill the pixmaps
  sidePixmap.fill(bgColor);
  sideTilePixmap.fill(bgColor);

  return true;
}

bool PanelKMenu::loadTopPixmap() {
  TDEConfig config("classicxapplet_rc");

  // Migration: copy old [TopPicture] entries to [KMenu] group if needed
  if (config.hasGroup("TopPicture")) {
    config.setGroup("TopPicture");
    if (config.hasKey("TopPicShowText") || config.hasKey("TopPicMode")) {
      int m = config.readNumEntry("TopPicMode", 0);
      TQString emb = config.readEntry("TopPicEmbedded", "Royal");
      TQString cLeft = config.readEntry("TopPicCustomLeft", "");
      TQString cCenter = config.readEntry("TopPicCustomCenter", "");
      TQString cRight = config.readEntry("TopPicCustomRight", "");
      bool colorize = config.readBoolEntry("TopPicColorize", false);
      TQString col = config.readEntry("TopPicColor", "");
      bool showText = config.readBoolEntry("TopPicShowText", false);
      bool showUser = config.readBoolEntry("TopPicShowUser", false);
      bool showCustomText = config.readBoolEntry("TopPicShowCustomText", true);
      TQString text = config.readEntry("TopPicText", "Trinity Desktop");
      int textColorMode = config.readNumEntry("TopPicTextColorMode", 0);
      TQString textColor = config.readEntry("TopPicTextColor", "");
      bool showDate = config.readBoolEntry("TopPicShowDate", false);
      bool showTime = config.readBoolEntry("TopPicShowTime", false);

      config.setGroup("KMenu");
      if (!config.hasKey("TopPicShowText")) config.writeEntry("TopPicShowText", showText);
      if (!config.hasKey("TopPicMode")) config.writeEntry("TopPicMode", m);
      if (!config.hasKey("TopPicEmbedded")) config.writeEntry("TopPicEmbedded", emb);
      if (!config.hasKey("TopPicCustomLeft")) config.writeEntry("TopPicCustomLeft", cLeft);
      if (!config.hasKey("TopPicCustomCenter")) config.writeEntry("TopPicCustomCenter", cCenter);
      if (!config.hasKey("TopPicCustomRight")) config.writeEntry("TopPicCustomRight", cRight);
      if (!config.hasKey("TopPicColorize")) config.writeEntry("TopPicColorize", colorize);
      if (!config.hasKey("TopPicColor")) config.writeEntry("TopPicColor", col);
      if (!config.hasKey("TopPicShowUser")) config.writeEntry("TopPicShowUser", showUser);
      if (!config.hasKey("TopPicShowCustomText")) config.writeEntry("TopPicShowCustomText", showCustomText);
      if (!config.hasKey("TopPicText")) config.writeEntry("TopPicText", text);
      if (!config.hasKey("TopPicTextColorMode")) config.writeEntry("TopPicTextColorMode", textColorMode);
      if (!config.hasKey("TopPicTextColor")) config.writeEntry("TopPicTextColor", textColor);
      if (!config.hasKey("TopPicShowDate")) config.writeEntry("TopPicShowDate", showDate);
      if (!config.hasKey("TopPicShowTime")) config.writeEntry("TopPicShowTime", showTime);
      config.sync();
    }
  }

  // Refresh in-memory skeleton settings from disk
  ClassicXSettings::self()->readConfig();

  config.setGroup("KMenu");

  int mode = config.readNumEntry("TopPicMode",
                                 0); // 0 = None, 1 = Embedded, 2 = Custom
  if (mode == 0) {
    topPixLeft = topPixCenter = topPixRight = TQPixmap();
    topPixHeight = 0;
    return false;
  }

  bool invert = config.readBoolEntry("TopPicInvert", false);
  bool colorize = config.readBoolEntry("TopPicColorize", false);
  TQColor topColor = TQColor(config.readEntry(
      "TopPicColor", TDEGlobalSettings::highlightColor().name()));
  if (!topColor.isValid())
    topColor = TDEGlobalSettings::highlightColor();

  if (mode == 1) { // Embedded
    TQString themeName = config.readEntry("TopPicEmbedded", "Royal").lower();
    if (themeName.isEmpty())
      themeName = "royal";

    TQImage imgLeft = EmbeddedIcons::getNativeImage(themeName + "_left");
    TQImage imgCenter = EmbeddedIcons::getNativeImage(themeName + "_center");
    TQImage imgRight = EmbeddedIcons::getNativeImage(themeName + "_right");

    if (imgLeft.isNull() || imgCenter.isNull() || imgRight.isNull()) {
      topPixLeft = topPixCenter = topPixRight = TQPixmap();
      topPixHeight = 0;
      return false;
    }

    if (invert) {
      EmbeddedIcons::invertImage(imgLeft);
      EmbeddedIcons::invertImage(imgCenter);
      EmbeddedIcons::invertImage(imgRight);
    }

    if (colorize) {
      EmbeddedIcons::colorizeImage(imgLeft, topColor, invert);
      EmbeddedIcons::colorizeImage(imgCenter, topColor, invert);
      EmbeddedIcons::colorizeImage(imgRight, topColor, invert);
    }

    if (!imgLeft.isNull()) {
      topPixLeft.convertFromImage(imgLeft);
      if (imgLeft.hasAlphaBuffer()) {
        TQBitmap mask;
        mask.convertFromImage(imgLeft.createAlphaMask());
        topPixLeft.setMask(mask);
      }
    } else
      topPixLeft = TQPixmap();

    if (!imgCenter.isNull()) {
      topPixCenter.convertFromImage(imgCenter);
      if (imgCenter.hasAlphaBuffer()) {
        TQBitmap mask;
        mask.convertFromImage(imgCenter.createAlphaMask());
        topPixCenter.setMask(mask);
      }
    } else
      topPixCenter = TQPixmap();

    if (!imgRight.isNull()) {
      topPixRight.convertFromImage(imgRight);
      if (imgRight.hasAlphaBuffer()) {
        TQBitmap mask;
        mask.convertFromImage(imgRight.createAlphaMask());
        topPixRight.setMask(mask);
      }
    } else
      topPixRight = TQPixmap();
  } else if (mode == 2) { // Custom
    TQString leftPath = config.readEntry("TopPicCustomLeft", "");
    TQString centerPath = config.readEntry("TopPicCustomCenter", "");
    TQString rightPath = config.readEntry("TopPicCustomRight", "");

    TQImage imgLeft(leftPath);
    TQImage imgCenter(centerPath);
    TQImage imgRight(rightPath);

    if (imgLeft.isNull() && imgCenter.isNull() && imgRight.isNull()) {
      topPixLeft = topPixCenter = topPixRight = TQPixmap();
      topPixHeight = 0;
      return false;
    }

    if (invert) {
      if (!imgLeft.isNull())
        EmbeddedIcons::invertImage(imgLeft);
      if (!imgCenter.isNull())
        EmbeddedIcons::invertImage(imgCenter);
      if (!imgRight.isNull())
        EmbeddedIcons::invertImage(imgRight);
    }

    if (colorize) {
      if (!imgLeft.isNull())
        EmbeddedIcons::colorizeImage(imgLeft, topColor, invert);
      if (!imgCenter.isNull())
        EmbeddedIcons::colorizeImage(imgCenter, topColor, invert);
      if (!imgRight.isNull())
        EmbeddedIcons::colorizeImage(imgRight, topColor, invert);
    }

    if (!imgLeft.isNull()) {
      topPixLeft.convertFromImage(imgLeft);
      if (imgLeft.hasAlphaBuffer()) {
        TQBitmap mask;
        mask.convertFromImage(imgLeft.createAlphaMask());
        topPixLeft.setMask(mask);
      }
    } else
      topPixLeft = TQPixmap();

    if (!imgCenter.isNull()) {
      topPixCenter.convertFromImage(imgCenter);
      if (imgCenter.hasAlphaBuffer()) {
        TQBitmap mask;
        mask.convertFromImage(imgCenter.createAlphaMask());
        topPixCenter.setMask(mask);
      }
    } else
      topPixCenter = TQPixmap();

    if (!imgRight.isNull()) {
      topPixRight.convertFromImage(imgRight);
      if (imgRight.hasAlphaBuffer()) {
        TQBitmap mask;
        mask.convertFromImage(imgRight.createAlphaMask());
        topPixRight.setMask(mask);
      }
    } else
      topPixRight = TQPixmap();
  }

  int hLeft = topPixLeft.isNull() ? 0 : topPixLeft.height();
  int hCenter = topPixCenter.isNull() ? 0 : topPixCenter.height();
  int hRight = topPixRight.isNull() ? 0 : topPixRight.height();

  topPixHeight = hLeft;
  if (hCenter > topPixHeight)
    topPixHeight = hCenter;
  if (hRight > topPixHeight)
    topPixHeight = hRight;

  return topPixHeight > 0;
}

void PanelKMenu::updateTopPicMask() {
  if (topPixHeight <= 0) {
    m_hasTopPicMask = false;
    m_baseTopPicMask = TQRegion();
    clearMask();
    return;
  }

  bool hasAlpha = (topPixLeft.mask() != 0 || topPixRight.mask() != 0 || topPixCenter.mask() != 0);
  m_hasTopPicMask = hasAlpha;

  if (!hasAlpha) {
    m_baseTopPicMask = TQRegion();
    clearMask();
    return;
  }

  TQRegion maskRegion(
      TQRect(0, topPixHeight, width(), height() - topPixHeight));

  int wLeft = topPixLeft.isNull() ? 0 : topPixLeft.width();
  int wRight = topPixRight.isNull() ? 0 : topPixRight.width();
  int wTotal = width();
  int centerWidth = wTotal - wLeft - wRight;

  if (!topPixLeft.isNull()) {
    if (topPixLeft.mask()) {
      maskRegion = maskRegion.unite(TQRegion(*topPixLeft.mask()));
    } else {
      maskRegion = maskRegion.unite(TQRect(0, 0, wLeft, topPixLeft.height()));
    }
  }

  if (!topPixRight.isNull()) {
    int xR = wTotal - wRight;
    if (topPixRight.mask()) {
      TQRegion rightReg(*topPixRight.mask());
      rightReg.translate(xR, 0);
      maskRegion = maskRegion.unite(rightReg);
    } else {
      maskRegion =
          maskRegion.unite(TQRect(xR, 0, wRight, topPixRight.height()));
    }
  }

  if (!topPixCenter.isNull() && centerWidth > 0) {
    if (topPixCenter.mask()) {
      for (int x = wLeft; x < wTotal - wRight; x += topPixCenter.width()) {
        TQRegion centerReg(*topPixCenter.mask());
        centerReg.translate(x, 0);
        maskRegion = maskRegion.unite(centerReg);
      }
    } else {
      maskRegion = maskRegion.unite(
          TQRect(wLeft, 0, centerWidth, topPixCenter.height()));
    }
  }

  m_baseTopPicMask = maskRegion;
  setMask(maskRegion);
}

void PanelKMenu::paletteChanged() {
  if (!loadSidePixmap()) {
    sidePixmap = sideTilePixmap = TQPixmap();
    setMinimumSize(sizeHint());
  }
  loadTopPixmap();
}

void PanelKMenu::ensureSessionsMenu() {
  if (!sessionsMenu && DM::isSwitchableCached() && kapp->authorize("switch_user")) {
    sessionsMenu = new TQPopupMenu(this);
    sessionsMenu->installEventFilter(this);
    connect(sessionsMenu, TQT_SIGNAL(aboutToShow()),
            TQT_SLOT(slotPopulateSessions()));
    connect(sessionsMenu, TQT_SIGNAL(activated(int)),
            TQT_SLOT(slotSessionActivated(int)));
    KickerLib::updateMenuPalette(sessionsMenu);
  }
}

void PanelKMenu::ensureLogoutMenu() {
  if (logoutMenu)
    return;
  logoutMenu = new TQPopupMenu(this);
  connect(logoutMenu, TQT_SIGNAL(aboutToShow()), TQT_SLOT(slotPopulateLogout()));
  connect(logoutMenu, TQT_SIGNAL(activated(int)), TQT_SLOT(slotSuspend(int)));
  logoutMenu->installEventFilter(this);
  KickerLib::updateMenuPalette(logoutMenu);
}

void PanelKMenu::resetSidebarPopups() {
  if (sessionsMenu) {
    sessionsMenu->hide();
    delete sessionsMenu;
    sessionsMenu = 0;
  }
  if (logoutMenu) {
    logoutMenu->hide();
    delete logoutMenu;
    logoutMenu = 0;
  }
  m_inPopulateSessions = false;
  m_inPopulateLogout = false;
}

int PanelKMenu::userShutdownPopupHeight() const {
  int targetH = ClassicXSettings::fullUserShutdownMenuHeight()
                    ? height()
                    : ClassicXSettings::customUserShutdownMenuHeight();
  if (targetH > height() && height() >= 200)
    targetH = height();
  if (targetH < 200)
    targetH = 200;
  return targetH;
}

void PanelKMenu::applyUserShutdownPopupSize(TQPopupMenu *menu, int targetH) {
  if (!menu || targetH <= 0)
    return;
  int sw = sidePixmap.width() > 0 ? sidePixmap.width()
                                  : ClassicXSettings::sideBarWidth();
  int minW = width() - sw + 6;
  if (minW < 100)
    minW = 100;
  menu->setMinimumWidth(minW);
  menu->setMaximumWidth(minW);
  menu->setMinimumHeight(targetH);
  menu->setMaximumHeight(targetH);
  menu->resize(minW, targetH);
}

void PanelKMenu::initialize() {
  if (initialized()) {
    return;
  }

  RecentlyLaunchedApps::the().init();

  loadTopPixmap();

  if (loadSidePixmap()) {
    // in case we've been through here before, let's disconnect
    disconnect(kapp, TQT_SIGNAL(tdedisplayPaletteChanged()), this,
               TQT_SLOT(paletteChanged()));
    connect(kapp, TQT_SIGNAL(tdedisplayPaletteChanged()), this,
            TQT_SLOT(paletteChanged()));
  } else {
    sidePixmap = sideTilePixmap = TQPixmap();
  }

  // SAFE RE-INITIALIZATION:
  // The search bar is NOT a menu item (not inserted via insertItem),
  // so TQPopupMenu::clear() will not delete it. We still detach it
  // from the widget tree during reinitialization for safety.
  TQHBox *hbox_to_detach =
      (searchEdit) ? dynamic_cast<TQHBox *>(searchEdit->parent()) : 0;
  if (hbox_to_detach) {
    hbox_to_detach->reparent(NULL, TQPoint(0, 0));
  }

  // add services
  PanelServiceMenu::initialize();

  if (topPixHeight > 0) {
    int spacerId = insertItem(new TopSpacerMenuItem(topPixHeight), -1, 0);
    setItemEnabled(spacerId, false);
  }

  // TQToolTip::add(clearButton, i18n("Clear Search"));
  // TQToolTip::add(searchEdit, i18n("Enter the name of an application"));

  // create recent menu section
  createRecentMenuItems();

  insertSeparator();

  // insert bookmarks
  if (ClassicXSettings::useBookmarks() &&
      kapp->authorizeTDEAction("bookmarks")) {
    // Need to create a new popup each time, it's deleted by subMenus.clear()
    TDEPopupMenu *bookmarkParent = new TDEPopupMenu(this, "bookmarks");
    KickerLib::updateMenuPalette(bookmarkParent);
    if (!bookmarkOwner)
      bookmarkOwner = new KBookmarkOwner;
    delete bookmarkMenu; // can't reuse old one, the popup has been deleted
    // Use KBookmarkManager directly — avoids libkonq (KonqBookmarkManager).
    KBookmarkManager *bmMgr = KBookmarkManager::managerForFile(locateLocal(
        "data", TQString::fromLatin1("konqueror/bookmarks.xml"), true));
    bookmarkMenu = new KBookmarkMenu(bmMgr, bookmarkOwner, bookmarkParent,
                                     actionCollection, true, false);

    insertItem(KickerLib::menuIconSet("bookmark"), i18n("Bookmarks"),
               bookmarkParent);

    subMenus.append(bookmarkParent);
  }

  // insert quickbrowser
  if (ClassicXSettings::useBrowser()) {
    PanelQuickBrowser *browserMnu = new PanelQuickBrowser(this);
    browserMnu->initialize();
    KickerLib::updateMenuPalette(browserMnu);

    insertItem(KickerLib::menuIconSet("kdisknav"), i18n("Quick Browser"),
               browserMnu);
    subMenus.append(browserMnu);
  }

  // insert dynamic menus
  TDESharedConfig::Ptr kickerCfg = TDESharedConfig::openConfig("kickerrc");
  kickerCfg->reparseConfiguration();
  kickerCfg->setGroup("menus");
  TQStringList menu_ext = kickerCfg->readListEntry("Extensions");
  if (menu_ext.isEmpty()) {
    menu_ext = ClassicXSettings::menuExtensions();
  }
  if (!menu_ext.isEmpty()) {
    for (TQStringList::ConstIterator it = menu_ext.begin();
         it != menu_ext.end(); ++it) {
      if (!ClassicXSettings::showControlCenter() && *it == "prefmenu.desktop")
        continue; // Exclude "Trinity Control Center" if disabled
      MenuInfo info(*it);
      if (!info.isValid())
        continue;

      KPanelMenu *menu = info.load();
      if (menu) {
        KickerLib::updateMenuPalette(menu);
        TQIconSet iconset = KickerLib::menuIconSet(info.icon());
        if (iconset.isNull())
          insertItem(info.name(), menu);
        else
          insertItem(iconset, info.name(), menu);
        dynamicSubMenus.append(menu);
      }
    }
  }

  // insert client menus, if any
  if (clients.count() > 0) {
    TQIntDictIterator<KickerClientMenu> it(clients);
    while (it) {
      if (it.current()->text.at(0) != '.')
        insertItem(it.current()->icon, it.current()->text, it.current(),
                   it.currentKey());
      ++it;
    }
    insertSeparator();
  }

  // run command
  if (kapp->authorize("run_command") && ClassicXSettings::showRunCommand()) {
    insertItem(KickerLib::menuIconSet("system-run"), i18n("Run Command..."),
               this, TQT_SLOT(slotRunCommand()));
  }

  // Switch User: create sessions submenu (will be shown from sidebar icon click)
  ensureSessionsMenu();

  /*
    If  the user configured ksmserver to
  */
  TDEConfig ksmserver("ksmserverrc", false, false);
  ksmserver.setGroup("General");
  if (ksmserver.readEntry("loginMode") == "restoreSavedSession") {
    insertItem(KickerLib::menuIconSet("document-save"), i18n("Save Session"),
               this, TQT_SLOT(slotSaveSession()));
  }

  // Special item: User Menu in main menu tree (only if not active on sidebar)
  bool userInSidebar = ClassicXSettings::useSidePixmap() &&
                       ClassicXSettings::showSidebarUserMenu();
  if (ClassicXSettings::showSpecialUserMenu() && !userInSidebar &&
      DM::isSwitchableCached() && kapp->authorize("switch_user")) {
    treeUserMenu = new TQPopupMenu(this);
    treeUserMenu->installEventFilter(this);
    connect(treeUserMenu, TQT_SIGNAL(aboutToShow()),
            TQT_SLOT(slotPopulateTreeUserMenu()));
    connect(treeUserMenu, TQT_SIGNAL(activated(int)),
            TQT_SLOT(slotTreeUserMenuActivated(int)));
    KickerLib::updateMenuPalette(treeUserMenu);

    KUser currentUser;
    TQString userName = currentUser.fullName();
    if (userName.isEmpty())
      userName = currentUser.loginName();

    insertItem(EmbeddedIcons::getSmallIconSet("switchuser"), userName,
               treeUserMenu);
    subMenus.append(treeUserMenu);
  }

  // Special item: Shutdown Menu in main menu tree (only if not active on
  // sidebar)
  bool shutdownInSidebar = ClassicXSettings::useSidePixmap() &&
                           ClassicXSettings::showSidebarShutdownMenu();
  if (ClassicXSettings::showSpecialShutdownMenu() && !shutdownInSidebar) {
    treeShutdownMenu = new TQPopupMenu(this);
    treeShutdownMenu->installEventFilter(this);
    connect(treeShutdownMenu, TQT_SIGNAL(aboutToShow()),
            TQT_SLOT(slotPopulateTreeShutdownMenu()));
    connect(treeShutdownMenu, TQT_SIGNAL(activated(int)),
            TQT_SLOT(slotSuspend(int)));
    KickerLib::updateMenuPalette(treeShutdownMenu);

    insertItem(EmbeddedIcons::getSmallIconSet("kickermenu-logout"),
               i18n("Shutdown"), treeShutdownMenu);
    subMenus.append(treeShutdownMenu);
  }

  /*
  if (kapp->authorize("lock_screen"))
  {
      insertItem(KickerLib::menuIconSet("system-lock-screen"), i18n("Lock
  Session"), this, TQT_SLOT(slotLock()));
  }
  */

#if 0
    // WABA: tear off handles don't work together with dynamically updated
    // menus. We can't update the menu while torn off, and we don't know
    // when it is torn off.
    if (TDEGlobalSettings::insertTearOffHandle())
      insertTearOffHandle();
#endif

  // Legacy repair timer start commented out
  // if (ClassicXSettings::useSidePixmap() && displayRepaired == false) {
  //     displayRepairTimer->start(5, true);
  //     displayRepaired = true;
  // }

  if (ClassicXSettings::useSearchBar()) {
    ensureSearchEdit();
    if (searchEdit) {
      TQHBox *hbox = dynamic_cast<TQHBox *>(searchEdit->parent());
      if (hbox) {
        hbox->reparent(this, TQPoint(0, 0)); // Keep it alive
        if (ClassicXSettings::alwaysShowSearchBar()) {
          hbox->show();
          if (indexOf(searchLineID) < 0)
            insertItem(hbox, searchLineID);
          setItemVisible(searchLineID, true);
        } else {
          if (indexOf(searchLineID) >= 0)
            setItemVisible(searchLineID, false);
          hbox->hide();
        }
      }
      updateSearchEditPalette();
    }
  }

  setInitialized(true);
  KickerLib::updateMenuPalette(this);
  setMinimumWidth(0);
  setMaximumWidth(30000);
  adjustSize();

  int currentSideW =
      (ClassicXSettings::useSidePixmap() && sidePixmap.width() > 0)
          ? sidePixmap.width()
          : (ClassicXSettings::useSidePixmap()
                 ? ClassicXSettings::sideBarWidth()
                 : 0);

  if (m_baseContentFloorWidth <= 0 && !m_inFlatSearchMode) {
    int extraPadding = ClassicXSettings::useSidePixmap() ? 80 : 0;
    m_baseContentFloorWidth = (width() - currentSideW) + extraPadding;
  }

  int floorW = (m_baseContentFloorWidth > 0) ? m_baseContentFloorWidth
                                             : (width() - currentSideW);
  int totalFloorW = floorW + currentSideW;
  if (ClassicXSettings::menuMinWidth() > 0) {
    totalFloorW = kMax(totalFloorW, ClassicXSettings::menuMinWidth());
  }
  if (m_widthBeforeSearch > 0) {
    totalFloorW = kMax(totalFloorW, m_widthBeforeSearch);
  }

  setMinimumWidth(kMax(width(), totalFloorW));
  setMaximumWidth(30000);
  adjustSize();
  updateGeometry();
  updateActiveSidebarButtons();
}

/*
void PanelKMenu::repairDisplay(void) {
    // Obsolete legacy repair hack - kept for history
}
*/

int PanelKMenu::insertClientMenu(KickerClientMenu *p) {
  int id = client_id;
  clients.insert(id, p);
  slotClear();
  return id;
}

void PanelKMenu::removeClientMenu(int id) {
  clients.remove(id);
  removeItem(id);
  slotClear();
}

extern int kicker_screen_number;

void PanelKMenu::slotLock() {
  TQCString appname("kdesktop");
  if (kicker_screen_number) {
    appname.sprintf("kdesktop-screen-%d", kicker_screen_number);
  }
  if (kapp && kapp->dcopClient()) {
    kapp->dcopClient()->send(appname, "KScreensaverIface", "lock()", TQByteArray());
  }
}

void PanelKMenu::slotLogout() {
  hide();
  kapp->requestShutDown();
}

void PanelKMenu::slotPopulateSessions() {
  if (m_inPopulateSessions)
    return;
  if (!DM::isSwitchableCached())
    return;
  m_inPopulateSessions = true;

  int p = 0;
  DM dm;

  sessionsMenu->setMinimumHeight(0);
  sessionsMenu->setMaximumHeight(32767);
  sessionsMenu->setMinimumWidth(0);
  sessionsMenu->setMaximumWidth(32767);
  sessionsMenu->clear();

  // Title at top
  KUser currentUser;
  TQString userName = currentUser.fullName();
  if (userName.isEmpty())
    userName = currentUser.loginName();
  sessionsMenu->insertItem(new PopupMenuTitle(userName, font()), 97, -1);

  // 1 padding item after title
  TQIconSet padIcon = KickerLib::menuIconSet("search_empty");
  int pad1 = sessionsMenu->insertItem(padIcon, TQString(" "), -1, -1);
  sessionsMenu->setItemEnabled(pad1, false);

  // Lock Session (moved from main menu)
  if (kapp->authorize("lock_screen")) {
    sessionsMenu->insertItem(
        EmbeddedIcons::getSmallIconSet("system-lock-screen"),
        i18n("Lock Session"), 99);
  }

  if (kapp->authorize("start_new_session") && (p = dm.numReserve()) >= 0) {
    sessionsMenu->insertItem(EmbeddedIcons::getSmallIconSet("switchuser"),
                             i18n("Start New Session"), 101);
    if (!p) {
      sessionsMenu->setItemEnabled(101, false);
    }
  }
  m_currentSessionVt = -1;
  SessList sess;
  if (dm.localSessions(sess)) {
    for (SessList::ConstIterator it = sess.begin(); it != sess.end(); ++it) {
      TQIconSet ico =
          (*it).self ? EmbeddedIcons::getSmallIconSet("check") : padIcon;
      int id = sessionsMenu->insertItem(ico, DM::sess2Str(*it), (*it).vt);
      if (!(*it).vt) {
        sessionsMenu->setItemEnabled(id, false);
      }
      if ((*it).self) {
        m_currentSessionVt = (*it).vt;
      }
    }
  }

  // Log Out (end session) at the bottom of the sessions list
  sessionsMenu->insertSeparator();
  sessionsMenu->insertItem(EmbeddedIcons::getSmallIconSet("menu-logout"),
                           i18n("Log Out"), 98);

  int targetH = userShutdownPopupHeight();
  padPopupToHeight(sessionsMenu, padIcon, targetH);
  applyUserShutdownPopupSize(sessionsMenu, targetH);

  m_inPopulateSessions = false;
}

void PanelKMenu::slotInitPowerSystem() {
  if (m_powerSystemInitialized)
    return;

#if defined(WITH_TDEHWLIB)
  TDERootSystemDevice *rootDevice =
      TDEGlobal::hardwareDevices()
          ? TDEGlobal::hardwareDevices()->rootSystemDevice()
          : 0;
  if (rootDevice) {
    m_canShutdown = rootDevice->canPowerOff();
    m_canFreeze = rootDevice->canFreeze();
    m_canSuspend = rootDevice->canSuspend();
    m_canHibernate = rootDevice->canHibernate();
    m_canHybridSuspend = rootDevice->canHybridSuspend();
  } else
#endif
  {
    m_canShutdown = true;
    m_canSuspend = true;
    m_canHibernate = true;
  }
  m_powerSystemInitialized = true;
}

void PanelKMenu::slotPopulateLogout() {
  if (m_inPopulateLogout)
    return;
  m_inPopulateLogout = true;

  logoutMenu->setMinimumHeight(0);
  logoutMenu->setMaximumHeight(32767);
  logoutMenu->setMinimumWidth(0);
  logoutMenu->setMaximumWidth(32767);
  logoutMenu->clear();

  // Title at top
  logoutMenu->insertItem(new PopupMenuTitle(i18n("Shutdown"), font()), 96, -1);

  // 1 padding item after title
  TQIconSet padIcon = KickerLib::menuIconSet("search_empty");
  int pad1 = logoutMenu->insertItem(padIcon, TQString(" "), -1, -1);
  logoutMenu->setItemEnabled(pad1, false);

  // Ensure power system is initialized (fallback if timer didn't fire yet)
  if (!m_powerSystemInitialized) {
    slotInitPowerSystem();
  }

  // Shutdown / Restart (respect hardware capabilities & user settings)
  bool showShutdown = m_canShutdown && ClassicXSettings::showShutdownPowerOff();
  bool showReboot = m_canShutdown && ClassicXSettings::showShutdownReboot();

  if (showShutdown) {
    logoutMenu->insertItem(EmbeddedIcons::getSmallIconSet("kickermenu-logout"),
                           i18n("Shutdown"), this, TQT_SLOT(slotShutdown()));
  }
  if (showReboot) {
    logoutMenu->insertItem(EmbeddedIcons::getSmallIconSet("menu-restart"),
                           i18n("Restart"), this, TQT_SLOT(slotReboot()));
  }

  // Suspend / Hibernate options (respect power-manager settings & user
  // settings)
  const PowerManagerFlags &pm = powerManagerFlags();
  bool showFreeze = m_canFreeze && !pm.disableSuspend;
  bool showSuspend = m_canSuspend && !pm.disableSuspend &&
                     ClassicXSettings::showShutdownSuspend();
  bool showHibernate = m_canHibernate && !pm.disableHibernate &&
                       ClassicXSettings::showShutdownHibernate();
  bool showHybrid = m_canHybridSuspend && !pm.disableSuspend &&
                    !pm.disableHibernate &&
                    ClassicXSettings::showShutdownHybridSuspend();

  bool anyTopSection = showShutdown || showReboot;
  bool anyBottomSection =
      showFreeze || showSuspend || showHibernate || showHybrid;

  if (anyTopSection && anyBottomSection) {
    logoutMenu->insertSeparator();
  }
  if (showFreeze) {
    logoutMenu->insertItem(EmbeddedIcons::getSmallIconSet("kickermenu-logout"),
                           i18n("Freeze"), SuspendType::Freeze);
  }
  if (showSuspend) {
    logoutMenu->insertItem(EmbeddedIcons::getSmallIconSet("menu-sleep"),
                           i18n("Suspend"), SuspendType::Suspend);
  }
  if (showHibernate) {
    logoutMenu->insertItem(EmbeddedIcons::getSmallIconSet("menu-hibernate"),
                           i18n("Hibernate"), SuspendType::Hibernate);
  }
  if (showHybrid) {
    logoutMenu->insertItem(EmbeddedIcons::getSmallIconSet("menu-hybrid"),
                           i18n("Hybrid Suspend"), SuspendType::HybridSuspend);
  }

  int targetHLogout = userShutdownPopupHeight();
  padPopupToHeight(logoutMenu, padIcon, targetHLogout);
  applyUserShutdownPopupSize(logoutMenu, targetHLogout);

  m_inPopulateLogout = false;
}

void PanelKMenu::slotPopulateTreeUserMenu() {
  if (!treeUserMenu)
    return;
  treeUserMenu->clear();

  if (kapp->authorize("lock_screen")) {
    treeUserMenu->insertItem(
        EmbeddedIcons::getSmallIconSet("system-lock-screen"),
        i18n("Lock Session"), 99);
  }

  DM dm;
  int p = -1;
  if (kapp->authorize("start_new_session") && (p = dm.numReserve()) >= 0) {
    int id =
        treeUserMenu->insertItem(EmbeddedIcons::getSmallIconSet("switchuser"),
                                 i18n("Start New Session"), 101);
    if (!p) {
      treeUserMenu->setItemEnabled(101, false);
    }
  }

  TQIconSet padIcon = KickerLib::menuIconSet("search_empty");
  SessList sess;
  if (dm.localSessions(sess)) {
    for (SessList::ConstIterator it = sess.begin(); it != sess.end(); ++it) {
      TQIconSet ico =
          (*it).self ? EmbeddedIcons::getSmallIconSet("check") : padIcon;
      int id = treeUserMenu->insertItem(ico, DM::sess2Str(*it), (*it).vt);
      if (!(*it).vt) {
        treeUserMenu->setItemEnabled(id, false);
      }
    }
  }

  treeUserMenu->insertSeparator();
  treeUserMenu->insertItem(EmbeddedIcons::getSmallIconSet("menu-logout"),
                           i18n("Log Out"), 98);
}

void PanelKMenu::slotTreeUserMenuActivated(int id) {
  if (id == 99) {
    slotLock();
  } else if (id == 98) {
    slotLogout();
  } else if (id == 101) {
    slotSessionActivated(101);
  } else if (id > 0) {
    slotSessionActivated(id);
  }
}

void PanelKMenu::slotPopulateTreeShutdownMenu() {
  if (!treeShutdownMenu)
    return;
  treeShutdownMenu->clear();

  if (!m_powerSystemInitialized) {
    slotInitPowerSystem();
  }

  bool showShutdown = m_canShutdown && ClassicXSettings::showShutdownPowerOff();
  bool showReboot = m_canShutdown && ClassicXSettings::showShutdownReboot();

  if (showShutdown) {
    treeShutdownMenu->insertItem(
        EmbeddedIcons::getSmallIconSet("kickermenu-logout"), i18n("Shutdown"),
        this, TQT_SLOT(slotShutdown()));
  }
  if (showReboot) {
    treeShutdownMenu->insertItem(EmbeddedIcons::getSmallIconSet("menu-restart"),
                                 i18n("Restart"), this, TQT_SLOT(slotReboot()));
  }

  const PowerManagerFlags &pm = powerManagerFlags();
  bool showFreeze = m_canFreeze && !pm.disableSuspend;
  bool showSuspend = m_canSuspend && !pm.disableSuspend &&
                     ClassicXSettings::showShutdownSuspend();
  bool showHibernate = m_canHibernate && !pm.disableHibernate &&
                       ClassicXSettings::showShutdownHibernate();
  bool showHybrid = m_canHybridSuspend && !pm.disableSuspend &&
                    !pm.disableHibernate &&
                    ClassicXSettings::showShutdownHybridSuspend();

  bool anyTopSection = showShutdown || showReboot;
  bool anyBottomSection =
      showFreeze || showSuspend || showHibernate || showHybrid;

  if (anyTopSection && anyBottomSection) {
    treeShutdownMenu->insertSeparator();
  }
  if (showFreeze) {
    treeShutdownMenu->insertItem(
        EmbeddedIcons::getSmallIconSet("kickermenu-logout"), i18n("Freeze"),
        SuspendType::Freeze);
  }
  if (showSuspend) {
    treeShutdownMenu->insertItem(EmbeddedIcons::getSmallIconSet("menu-sleep"),
                                 i18n("Suspend"), SuspendType::Suspend);
  }
  if (showHibernate) {
    treeShutdownMenu->insertItem(
        EmbeddedIcons::getSmallIconSet("menu-hibernate"), i18n("Hibernate"),
        SuspendType::Hibernate);
  }
  if (showHybrid) {
    treeShutdownMenu->insertItem(EmbeddedIcons::getSmallIconSet("menu-hybrid"),
                                 i18n("Hybrid Suspend"),
                                 SuspendType::HybridSuspend);
  }
}

void PanelKMenu::slotControlCenter() {
  hide();
  TDEProcess *proc = new TDEProcess;
  connect(proc, TQT_SIGNAL(processExited(TDEProcess *)), proc,
          TQT_SLOT(deleteLater()));
  *proc << "konqueror" << "--profile" << "ctrl_panel";
  if (!proc->start(TDEProcess::DontCare)) {
    delete proc;
  }
}

void PanelKMenu::slotShutdown() {
  hide();
  TQByteArray params;
  TQDataStream stream(params, IO_WriteOnly);
  stream << (int)2 << (int)-1 << TQString("");
  kapp->dcopClient()->send("ksmserver", "default",
                           "logoutTimed(int,int,TQString)", params);
}

void PanelKMenu::slotReboot() {
  hide();
  TQByteArray params;
  TQDataStream stream(params, IO_WriteOnly);
  stream << (int)1 << (int)-1 << TQString();
  kapp->dcopClient()->send("ksmserver", "default",
                           "logoutTimed(int,int,TQString)", params);
}

void PanelKMenu::slotSuspend(int id) {
  // Ignore title (ID 96) and non-suspend IDs
  if (id == 96 || id < 0)
    return;

  hide();

  // Screen lock left 100% to user's power manager
  // if (powerManagerFlags().lockOnResume) {
  //     DCOPRef("kdesktop", "KScreensaverIface").call("lock()");
  // }

  TDEProcess *proc = new TDEProcess;
  connect(proc, TQT_SIGNAL(processExited(TDEProcess *)), proc,
          TQT_SLOT(deleteLater()));

  if (id == SuspendType::Freeze) {
    *proc << "systemctl" << "suspend";
  } else if (id == SuspendType::Suspend) {
    *proc << "systemctl" << "suspend";
  } else if (id == SuspendType::Hibernate) {
    *proc << "systemctl" << "hibernate";
  } else if (id == SuspendType::HybridSuspend) {
    *proc << "systemctl" << "hybrid-sleep";
  } else {
    delete proc;
    return;
  }

  if (!proc->start(TDEProcess::DontCare)) {
    delete proc;
    KMessageBox::error(this, i18n("Suspend failed"));
  }
}

void PanelKMenu::slotSessionActivated(int ent) {
  // Ignore title item (ID 97) and padding items — do nothing
  if (ent == 97 || ent < 0)
    return;

  // Close menus immediately to release input grabs and visual focus
  if (sessionsMenu)
    sessionsMenu->hide();
  hide();
  if (ent == 98)
    TQTimer::singleShot(0, this, TQT_SLOT(slotLogout()));
  else if (ent == 99)
    TQTimer::singleShot(0, this, TQT_SLOT(slotLock()));
  else if (ent == 101)
    doNewSession(true);
  else if (sessionsMenu && ent != m_currentSessionVt)
    DM().lockSwitchVT(ent);
}

void PanelKMenu::doNewSession(bool lock) {
  TQGuardedPtr<PanelKMenu> guard(this);
  int result = KMessageBox::warningContinueCancel(
      kapp->desktop()->screen(kapp->desktop()->screenNumber(this)),
      i18n("<p>You have chosen to open another desktop session.<br>"
           "The current session will be hidden "
           "and a new login screen will be displayed.<br>"
           "An F-key is assigned to each session; "
           "F%1 is usually assigned to the first session, "
           "F%2 to the second session and so on. "
           "You can switch between sessions by pressing "
           "Ctrl, Alt and the appropriate F-key at the same time. "
           "Additionally, the TDE Panel and Desktop menus have "
           "actions for switching between sessions.</p>")
          .arg(7)
          .arg(8),
      i18n("Warning - New Session"),
      KGuiItem(i18n("&Start New Session"), "fork"), ":confirmNewSession",
      KMessageBox::PlainCaption | KMessageBox::Notify);

  if (!guard || result == KMessageBox::Cancel)
    return;

  if (lock) {
    TQCString appname("kdesktop");
    if (kicker_screen_number) {
      appname.sprintf("kdesktop-screen-%d", kicker_screen_number);
    }
    TQCString replyType;
    TQByteArray replyData;
    bool ok = (kapp && kapp->dcopClient()) ?
      kapp->dcopClient()->call(appname, "KScreensaverIface", "lock()", TQByteArray(), replyType, replyData, false, 4000) : false;
    if (!ok) {
      return;
    }
  }

  DM().startReserve();
}

void PanelKMenu::slotSaveSession() {
  TQByteArray data;
  kapp->dcopClient()->send("ksmserver", "default", "saveCurrentSession()",
                           data);
}

void PanelKMenu::slotRunCommand() {
  TQByteArray data;
  TQCString appname("kdesktop");
  if (kicker_screen_number)
    appname.sprintf("kdesktop-screen-%d", kicker_screen_number);

  kapp->updateRemoteUserTimestamp(appname);
  kapp->dcopClient()->send(appname, "KDesktopIface", "popupExecuteCommand()",
                           data);
}

void PanelKMenu::slotEditUserContact() {}

void PanelKMenu::setMinimumSize(const TQSize &s) {
  KPanelMenu::setMinimumSize(s);
}

void PanelKMenu::setMaximumSize(const TQSize &s) {
  KPanelMenu::setMaximumSize(s);
}

void PanelKMenu::setMinimumSize(int w, int h) {
  KPanelMenu::setMinimumSize(w, h);
}

void PanelKMenu::setMaximumSize(int w, int h) {
  KPanelMenu::setMaximumSize(w, h);
}

void PanelKMenu::showMenu() {
  if (m_appletButton) {
    m_appletButton->showMenu();
  } else {
    show();
  }
}

TQRect PanelKMenu::sideImageRect() {
  int topH = (topPixHeight > 0) ? topPixHeight : 0;
  return TQStyle::visualRect(TQRect(frameWidth(), frameWidth() + topH,
                                    sidePixmap.width(),
                                    height() - 2 * frameWidth() - topH),
                             this);
}

void PanelKMenu::resizeEvent(TQResizeEvent *e) {
  PanelServiceMenu::resizeEvent(e);

  int sideW = (ClassicXSettings::useSidePixmap() && sidePixmap.width() > 0)
                  ? sidePixmap.width()
                  : 0;

  setFrameRect(
      TQStyle::visualRect(TQRect(sideW, 0, width() - sideW, height()), this));

  updateActiveSidebarButtons();
  updateTopPicMask();
}

// Workaround Qt3.3.x sizing bug, by ensuring we're always wide enough.
void PanelKMenu::resize(int width, int height) {
  int minW = ClassicXSettings::menuMinWidth();
  if (minW > 0 && width < minW) {
    width = minW;
  }
  PanelServiceMenu::resize(width, height);
}

TQSize PanelKMenu::sizeHint() const {
  TQSize s = PanelServiceMenu::sizeHint();
  int minW = ClassicXSettings::menuMinWidth();
  if (minW > 0 && s.width() < minW) {
    s.setWidth(minW);
  }
  return s;
}

void PanelKMenu::hideEvent(TQHideEvent *e) {
  if (m_openingAnimTimer) {
    m_openingAnimTimer->stop();
  }
  m_inOpeningAnim = false;

  if (m_sidebarPopupHoverTimer) {
    m_sidebarPopupHoverTimer->stop();
  }
  m_pendingHoverBtn = -1;
  m_blockedHoverBtn = -1;

  // Reset sidebar hover state when menu closes
  if (m_hoveredSidebarBtn != -1) {
    m_hoveredSidebarBtn = -1;
    update(sideImageRect());
  }

  if (sessionsMenu) {
    sessionsMenu->hide();
  }
  if (logoutMenu) {
    logoutMenu->hide();
  }

  // Defer stash restore: TDEPopupMenu often hide()s before emitting
  // activated(), and slotExec still needs the search entryMap_ entries.
  if (m_treeStashed)
    TQTimer::singleShot(0, this, TQT_SLOT(slotRestoreStashAfterHide()));

  // Call parent implementation
  PanelServiceMenu::hideEvent(e);
}

void PanelKMenu::showEvent(TQShowEvent *e) {
  PanelServiceMenu::showEvent(e);
  KickerLib::updateMenuPalette(this);
  if (ClassicXSettings::animateOpening() && !m_inFlatSearchMode) {
    m_inOpeningAnim = true;
    m_openingAnimStep = 0;
    m_totalOpeningAnimSteps = 16;

    if (ClassicXSettings::menuCentered()) {
      m_openingAnimDirection = KPanelApplet::Up;
    } else if (m_appletButton) {
      m_openingAnimDirection = m_appletButton->popupDirection();
    } else {
      m_openingAnimDirection = KPanelApplet::Up;
    }

    if (m_openingAnimDirection == KPanelApplet::Left || m_openingAnimDirection == KPanelApplet::Right) {
      m_openingAnimDistance = (width() < 220) ? (int)((float)width() * 0.75f) : 160;
    } else {
      m_openingAnimDistance = (height() < 220) ? (int)((float)height() * 0.75f) : 160;
    }

    int targetOpacity = ClassicXSettings::classicKMenuOpacity();
    int startOpacity = (int)((float)targetOpacity * 0.20f + 0.5f);
    if (startOpacity < 1) startOpacity = 1;
    ClassicX::applyWindowOpacity(winId(), startOpacity);

    // Apply initial step 0 mask immediately before the window is exposed on screen
    int drawX = 0, drawY = 0;
    TQRect clipRect(0, 0, width(), height());
    switch (m_openingAnimDirection) {
    case KPanelApplet::Up:
      drawY = m_openingAnimDistance;
      clipRect = TQRect(0, m_openingAnimDistance, width(), height() - m_openingAnimDistance);
      break;
    case KPanelApplet::Down:
      drawY = -m_openingAnimDistance;
      clipRect = TQRect(0, 0, width(), height() - m_openingAnimDistance);
      break;
    case KPanelApplet::Right:
      drawX = -m_openingAnimDistance;
      clipRect = TQRect(0, 0, width() - m_openingAnimDistance, height());
      break;
    case KPanelApplet::Left:
      drawX = m_openingAnimDistance;
      clipRect = TQRect(m_openingAnimDistance, 0, width() - m_openingAnimDistance, height());
      break;
    }

    if (m_hasTopPicMask && !m_baseTopPicMask.isEmpty()) {
      TQRegion animMask = m_baseTopPicMask;
      animMask.translate(drawX, drawY);
      setMask(animMask.intersect(clipRect));
    } else {
      setMask(TQRegion(clipRect));
    }

    m_openingAnimTimer->start(12);
  } else {
    m_inOpeningAnim = false;
    ClassicX::applyWindowOpacity(winId(), ClassicXSettings::classicKMenuOpacity());
    if (m_hasTopPicMask && !m_baseTopPicMask.isEmpty()) {
      setMask(m_baseTopPicMask);
    } else {
      clearMask();
    }
  }

  if (!m_inFlatSearchMode) {
    TQTimer::singleShot(0, this, TQT_SLOT(slotCaptureMainMenuGeometry()));
  }
}

void PanelKMenu::slotCaptureMainMenuGeometry() {
  if (isVisible() && !m_inFlatSearchMode && !m_treeStashed) {
    if (width() > 100) {
      m_widthBeforeSearch = width();
    }
    m_mainMenuPos = pos();
    m_mainMenuSize = size();
    TQStringList rList;
    RecentlyLaunchedApps::the().getRecentApps(rList);
    m_savedRecentCount = rList.count();
  }
}

void PanelKMenu::updateActiveSidebarButtons() {
  if (!ClassicXSettings::useSidePixmap()) {
    m_activeSidebarButtons.clear();
    m_lastSidebarHeight = -1;
    m_lastSidebarWidth = -1;
    m_lastSidebarIconSize = -1;
    m_lastSidebarAlign = -1;
    return;
  }

  int sw = sidePixmap.width() > 0 ? sidePixmap.width()
                                  : ClassicXSettings::sideBarWidth();
  int h = height();
  int iconSize = ClassicXSettings::uiIconSize();
  int align = ClassicXSettings::sidebarButtonsAlign();
  int userOnTop = ClassicXSettings::sidebarUserOnTop() ? 1 : 0;

  if (h == m_lastSidebarHeight && sw == m_lastSidebarWidth &&
      iconSize == m_lastSidebarIconSize && align == m_lastSidebarAlign &&
      userOnTop == m_lastSidebarUserOnTop &&
      !m_activeSidebarButtons.isEmpty()) {
    return;
  }

  m_lastSidebarHeight = h;
  m_lastSidebarWidth = sw;
  m_lastSidebarIconSize = iconSize;
  m_lastSidebarAlign = align;
  m_lastSidebarUserOnTop = userOnTop;

  m_activeSidebarButtons.clear();
  int fw = frameWidth();
  if (iconSize < 20)
    iconSize = 20;
  if (iconSize > sw - 2)
    iconSize = sw - 2;
  if (iconSize > 60)
    iconSize = 60;
  if (iconSize < 20)
    iconSize = 20;

  int btnHeight = 34;
  if (iconSize + 8 > btnHeight) {
    btnHeight = iconSize + 8;
  }
  int btnGap = 8;
  int bottomPadding = 10;
  int step = btnHeight + btnGap;

  // Order from bottom to top: Logout (1), Settings (2), Downloads (5), Pictures (3), Documents (4), User (0)
  int candidates[6] = {1, 2, 5, 3, 4, 0};
  int stackIndex = 0;

  for (int i = 0; i < 6; ++i) {
    int id = candidates[i];
    bool isEnabled = false;

    switch (id) {
    case 1: // Logout
      isEnabled = ClassicXSettings::showSidebarShutdownMenu() &&
                  kapp->authorize("logout");
      break;
    case 2: // Settings
      isEnabled = ClassicXSettings::showSidebarSettings();
      break;
    case 5: // Downloads
      isEnabled = ClassicXSettings::showSidebarDownloads();
      break;
    case 3: // Pictures / Images
      isEnabled = ClassicXSettings::showSidebarImages();
      break;
    case 4: // Documents
      isEnabled = ClassicXSettings::showSidebarDocuments();
      break;
    case 0: // Switch User
      isEnabled = ClassicXSettings::showSidebarUserMenu() &&
                  DM::isSwitchableCached() && kapp->authorize("switch_user");
      break;
    }

    if (isEnabled) {
      SidebarBtn btn;
      btn.id = id;
      if (id == 0 && userOnTop) {
        int topH = (topPixHeight > 0) ? topPixHeight : 0;
        btn.rect = TQRect(fw, fw + topH + 10, sw, btnHeight);
      } else {
        btn.rect = TQRect(
            fw, height() - fw - bottomPadding - btnHeight - step * stackIndex, sw,
            btnHeight);
        stackIndex++;
      }
      switch (id) {
      case 0:
        btn.icon = EmbeddedIcons::getSmallIcon("switchuser", iconSize);
        break;
      case 1:
        btn.icon =
            EmbeddedIcons::getPixmap("kickermenu-logout", iconSize, iconSize);
        break;
      case 2:
        btn.icon =
            EmbeddedIcons::getPixmap("menu-settings", iconSize, iconSize);
        break;
      case 5:
        btn.icon =
            EmbeddedIcons::getPixmap("menu-downloads", iconSize, iconSize);
        break;
      case 3:
        btn.icon = EmbeddedIcons::getPixmap("menu-images", iconSize, iconSize);
        break;
      case 4:
        btn.icon = EmbeddedIcons::getPixmap("menu-docs", iconSize, iconSize);
        break;
      }
      int align = ClassicXSettings::sidebarButtonsAlign();
      int iconOffX = 0;
      if (align == 1) { // Left
        iconOffX = 4;
      } else if (align == 2) { // Right
        iconOffX = KMAX(0, sw - btn.icon.width() - 4);
      } else { // Center (0)
        iconOffX = KMAX(0, (sw - btn.icon.width()) / 2);
      }
      int iconOffY = (btn.rect.height() - btn.icon.height()) / 2;
      btn.iconPos = TQPoint(btn.rect.x() + iconOffX, btn.rect.y() + iconOffY);

      m_activeSidebarButtons.append(btn);
    }
  }
}

static TQString getFreeRamString() {
  int fd = open("/proc/meminfo", O_RDONLY);
  if (fd >= 0) {
    char buf[512];
    ssize_t n = read(fd, buf, sizeof(buf) - 1);
    close(fd);
    if (n > 0) {
      buf[n] = '\0';
      const char *p = strstr(buf, "MemAvailable:");
      if (!p)
        p = strstr(buf, "MemFree:");
      if (p) {
        const char *numPtr = p;
        while (*numPtr && (*numPtr < '0' || *numPtr > '9'))
          numPtr++;
        unsigned long kb = 0;
        if (sscanf(numPtr, "%lu", &kb) == 1 && kb > 0) {
          double gb = (double)kb / 1048576.0;
          return TQString("RAM: %1 GB").arg(TQString::number(gb, 'f', 1));
        }
      }
    }
  }
  return TQString::null;
}

void PanelKMenu::drawMenu(TQPainter *p, const TQRect &clipRect) {
  if (!p) return;

  p->setFont(font());
  TQColor bgColor = KickerLib::getClassicKMenuBgColor();

  style().drawPrimitive(TQStyle::PE_PanelPopup, p,
                        TQRect(0, 0, width(), height()), colorGroup(),
                        TQStyle::Style_Default, TQStyleOption(frameWidth(), 0));

  if (ClassicXSettings::useSidePixmap()) {
    int sbWidth = sidePixmap.width() > 0 ? sidePixmap.width()
                                         : ClassicXSettings::sideBarWidth();
    TQPixmap sideTile =
        KickerLib::getSidebarTilePixmap(sbWidth, sideImageRect().height());
    if (!sideTile.isNull()) {
      p->drawTiledPixmap(sideImageRect(), sideTile);
    } else {
      TQRect r = sideImageRect();
      r.setBottom(r.bottom() - sidePixmap.height());
      if (r.intersects(clipRect)) {
        p->drawTiledPixmap(r, sideTilePixmap);
      }

      r = sideImageRect();
      r.setTop(r.bottom() - sidePixmap.height());
      if (r.intersects(clipRect)) {
        TQRect drawRect = r.intersect(clipRect);
        TQRect pixRect = drawRect;
        pixRect.moveBy(-r.left(), -r.top());
        p->drawPixmap(drawRect.topLeft(), sidePixmap, pixRect);
      }
    }
  }

  drawContents(p);

  if (topPixHeight > 0) {
    int wLeft = topPixLeft.isNull() ? 0 : topPixLeft.width();
    int wRight = topPixRight.isNull() ? 0 : topPixRight.width();
    int wTotal = width();
    int centerWidth = wTotal - wLeft - wRight;

    if (!topPixLeft.isNull()) {
      p->drawPixmap(0, 0, topPixLeft);
    }
    if (!topPixRight.isNull()) {
      p->drawPixmap(wTotal - wRight, 0, topPixRight);
    }
    if (!topPixCenter.isNull() && centerWidth > 0) {
      p->drawTiledPixmap(wLeft, 0, centerWidth, topPixHeight, topPixCenter);
    }

    if (ClassicXSettings::topPicShowText() && centerWidth > 10) {
      TQTime nowTime = TQTime::currentTime();
      int currentMinute = nowTime.minute();

      if (m_cachedTopPicTitle.isNull() || currentMinute != m_lastTopPicMinute) {
        m_lastTopPicMinute = currentMinute;

        int activeCount = 0;
        if (ClassicXSettings::topPicShowUser())
          activeCount++;
        if (ClassicXSettings::topPicShowCustomText() &&
            !ClassicXSettings::topPicText().isEmpty())
          activeCount++;
        if (ClassicXSettings::topPicShowRam())
          activeCount++;
        if (ClassicXSettings::topPicShowDate())
          activeCount++;
        if (ClassicXSettings::topPicShowTime())
          activeCount++;

        TQStringList parts;
        if (ClassicXSettings::topPicShowUser()) {
          static TQString s_cachedUserName;
          if (s_cachedUserName.isEmpty()) {
            KUser user;
            s_cachedUserName = user.fullName();
            if (s_cachedUserName.isEmpty())
              s_cachedUserName = user.loginName();
          }
          if (!s_cachedUserName.isEmpty())
            parts.append(s_cachedUserName);
        }
        if (ClassicXSettings::topPicShowCustomText() &&
            !ClassicXSettings::topPicText().isEmpty()) {
          parts.append(ClassicXSettings::topPicText());
        }
        if (ClassicXSettings::topPicShowRam()) {
          TQString ramStr = getFreeRamString();
          if (!ramStr.isEmpty())
            parts.append(ramStr);
        }
        if (ClassicXSettings::topPicShowDate()) {
          bool useShortFormat = (activeCount > 1);
          parts.append(TDEGlobal::locale()->formatDate(TQDate::currentDate(),
                                                       useShortFormat));
        }
        if (ClassicXSettings::topPicShowTime()) {
          parts.append(
              TDEGlobal::locale()->formatTime(nowTime, false));
        }

        m_cachedTopPicTitle = parts.join(" - ");
      }

      if (!m_cachedTopPicTitle.isEmpty()) {
        p->save();
        TQFont fontText = font();
        fontText.setBold(true);
        p->setFont(fontText);

        TQRect textRect(wLeft + 6, 0, centerWidth - 12, topPixHeight);
        TQColor textCol;
        int modeColor = ClassicXSettings::topPicTextColorMode();
        if (modeColor == 0) {
          textCol = TQApplication::palette().active().buttonText();
        } else if (modeColor == 1) {
          textCol = KickerLib::getMenuTitleFgColor();
        } else {
          textCol = ClassicXSettings::topPicTextColor().isValid()
                        ? ClassicXSettings::topPicTextColor()
                        : KickerLib::getMenuTitleFgColor();
        }
        p->setPen(textCol);
        int alignFlag = AlignVCenter | AlignHCenter | SingleLine;
        int alignSetting = ClassicXSettings::topPicTextAlign();
        if (alignSetting == 1) {
          alignFlag = AlignVCenter | AlignLeft | SingleLine;
        } else if (alignSetting == 2) {
          alignFlag = AlignVCenter | AlignRight | SingleLine;
        }
        p->drawText(textRect, alignFlag, m_cachedTopPicTitle);
        p->restore();
      }
    }
  }

  if (ClassicXSettings::useSidePixmap()) {
    TQColor highlightColor = bgColor.light(110);

    const TQValueList<SidebarBtn> &btns = getActiveSidebarButtons();
    for (TQValueList<SidebarBtn>::ConstIterator it = btns.begin();
         it != btns.end(); ++it) {
      if (m_hoveredSidebarBtn == (*it).id) {
        p->fillRect((*it).rect, highlightColor);
      }
      p->drawPixmap((*it).iconPos, (*it).icon);
    }
  }
}

void PanelKMenu::paintEvent(TQPaintEvent *e) {
  if (m_inOpeningAnim) {
    float t = (float)m_openingAnimStep / (float)m_totalOpeningAnimSteps;
    if (t > 1.0f) t = 1.0f;
    float factor = 1.0f - (1.0f - t) * (1.0f - t) * (1.0f - t);
    int offset = (int)((1.0f - factor) * (float)m_openingAnimDistance + 0.5f);

    int drawX = 0;
    int drawY = 0;
    TQRect clipRect(0, 0, width(), height());

    switch (m_openingAnimDirection) {
    case KPanelApplet::Up: // Bottom panel: slide UP from bottom
      drawY = offset;
      clipRect = TQRect(0, offset, width(), height() - offset);
      break;
    case KPanelApplet::Down: // Top panel: slide DOWN from top
      drawY = -offset;
      clipRect = TQRect(0, 0, width(), height() - offset);
      break;
    case KPanelApplet::Right: // Left panel: slide RIGHT from left
      drawX = -offset;
      clipRect = TQRect(0, 0, width() - offset, height());
      break;
    case KPanelApplet::Left: // Right panel: slide LEFT from right
      drawX = offset;
      clipRect = TQRect(offset, 0, width() - offset, height());
      break;
    }

    if (m_hasTopPicMask && !m_baseTopPicMask.isEmpty()) {
      TQRegion animMask = m_baseTopPicMask;
      if (offset > 0) {
        animMask.translate(drawX, drawY);
        setMask(animMask.intersect(clipRect));
      } else {
        setMask(m_baseTopPicMask);
      }
    } else {
      if (offset > 0) {
        setMask(TQRegion(clipRect));
      } else {
        clearMask();
      }
    }

    TQPixmap buf(size());
    buf.fill(KickerLib::getClassicKMenuBgColor());
    TQPainter bp(&buf);
    drawMenu(&bp, TQRect(0, 0, width(), height()));
    bp.end();

    TQPainter p(this);
    if (offset > 0) {
      p.setClipRect(clipRect);
    }
    p.drawPixmap(drawX, drawY, buf);

    if (m_openingAnimStep >= m_totalOpeningAnimSteps) {
      m_inOpeningAnim = false;
      m_openingAnimTimer->stop();
      ClassicX::applyWindowOpacity(winId(), ClassicXSettings::classicKMenuOpacity());
    }
    return;
  }

  TQPainter p(this);
  p.setClipRegion(e->region());
  drawMenu(&p, e->rect());
}

TQPixmap PanelKMenu::renderMenuSnapshot() {
  if (!initialized()) {
    initialize();
  } else if (RecentlyLaunchedApps::the().m_bNeedToUpdate) {
    refreshRecentSection();
    RecentlyLaunchedApps::the().m_bNeedToUpdate = false;
  }
  KickerLib::updateMenuPalette(this);
  if (ClassicXSettings::alwaysShowSearchBar()) {
    showSearchBarItem();
  }
  adjustSize();

  // Clear any active or hovered item state before capturing snapshot
  setActiveItem(-1);
  m_hoveredSidebarBtn = -1;

  TQPixmap pm(width(), height());
  pm.fill(KickerLib::getClassicKMenuBgColor());

  TQPainter p(&pm);
  drawMenu(&p, TQRect(0, 0, width(), height()));

  // Render any visible child widgets (such as search bar) onto the snapshot
  const TQObjectList *childrenList = children();
  if (childrenList) {
    TQObjectListIt it(*childrenList);
    TQObject *obj;
    while ((obj = it.current()) != 0) {
      ++it;
      if (obj->isWidgetType()) {
        TQWidget *w = static_cast<TQWidget *>(obj);
        if (!w->isHidden() && w->width() > 0 && w->height() > 0) {
          TQPixmap childPm = TQPixmap::grabWidget(w);
          if (!childPm.isNull()) {
            p.drawPixmap(w->pos(), childPm);
          }
        }
      }
    }
  }

  return pm;
}

TQMouseEvent PanelKMenu::translateMouseEvent(TQMouseEvent *e) {
  TQRect side = sideImageRect();

  if (!side.contains(e->pos()))
    return *e;

  TQPoint newpos(e->pos());
  TQApplication::reverseLayout() ? newpos.setX(newpos.x() - side.width())
                                 : newpos.setX(newpos.x() + side.width());
  TQPoint newglobal(e->globalPos());
  TQApplication::reverseLayout() ? newglobal.setX(newpos.x() - side.width())
                                 : newglobal.setX(newpos.x() + side.width());

  return TQMouseEvent(e->type(), newpos, newglobal, e->button(), e->state());
}

void PanelKMenu::slotShowClassicXSettings() {
  static TQGuardedPtr<ClassicXSettingsDialog> s_settingsDialog = 0;

  if (s_settingsDialog) {
    s_settingsDialog->show();
    s_settingsDialog->raise();
    s_settingsDialog->setActiveWindow();
    return;
  }

  hide();
  if (sessionsMenu)
    sessionsMenu->hide();
  if (logoutMenu)
    logoutMenu->hide();

  TQGuardedPtr<PanelKMenu> gThis = this;
  ClassicXSettingsDialog *dlg = new ClassicXSettingsDialog(0);
  TQGuardedPtr<ClassicXSettingsDialog> gDlg = dlg;
  s_settingsDialog = dlg;

  int result = dlg->exec();

  if (!gDlg) {
    s_settingsDialog = 0;
    return;
  }

  if (result == TQDialog::Accepted) {
    if (gThis) {
      gThis->m_baseContentFloorWidth = 0;
      if (gThis->m_appletButton) {
        gThis->m_appletButton->reloadIcon();
      }
      gThis->slotReinitialize();
    }
  }

  if (gDlg) {
    ClassicXSettingsDialog *d = gDlg;
    delete d;
  }
  s_settingsDialog = 0;
}

void PanelKMenu::slotContextMenu(int selected) {
  PanelServiceMenu::slotContextMenu(selected);
}

void PanelKMenu::showServiceInTree(KService *service)
{
  if (!service)
    return;
  m_pendingShowInTreeMenuId = service->menuId();
  if (m_pendingShowInTreeMenuId.isEmpty())
    m_pendingShowInTreeMenuId = service->storageId();
  if (m_pendingShowInTreeMenuId.isEmpty())
    return;

  // Leave mouseReleaseEvent / context popup before rebuilding the tree.
  TQTimer::singleShot(0, this, TQT_SLOT(slotShowInTreeDeferred()));
}

void PanelKMenu::slotShowInTreeDeferred()
{
  const TQString menuId = m_pendingShowInTreeMenuId;
  m_pendingShowInTreeMenuId = TQString::null;
  if (menuId.isEmpty())
    return;

  delete popupMenu_;
  popupMenu_ = 0;

  slotRestoreMainMenu();
  highlightMenuItem(menuId);
}

void PanelKMenu::mousePressEvent(TQMouseEvent *e) {
  int sw = sidePixmap.width() > 0 ? sidePixmap.width()
                                  : ClassicXSettings::sideBarWidth();
  int sideXMax = sw + frameWidth() + 4;

  if (ClassicXSettings::useSidePixmap() &&
      (e->pos().x() <= sideXMax || sideImageRect().contains(e->pos()))) {
    // Close any open submenus (but NOT the main KMenu or sidebar popups)
    int guard = 0;
    TQWidget *popup = TQApplication::activePopupWidget();
    while (popup && popup != this && popup != sessionsMenu &&
           popup != logoutMenu && guard < 10) {
      popup->hide();
      popup = TQApplication::activePopupWidget();
      ++guard;
    }

    TQValueList<SidebarBtn> btns = getActiveSidebarButtons();
    for (TQValueList<SidebarBtn>::ConstIterator it = btns.begin();
         it != btns.end(); ++it) {
      if ((*it).rect.contains(e->pos())) {
        switch ((*it).id) {
        case 0: // Switch User
          ensureSessionsMenu();
          if (sessionsMenu) {
            if (logoutMenu)
              logoutMenu->hide();
            m_sidebarPopupHoverTimer->stop();
            m_pendingHoverBtn = -1;
            m_blockedHoverBtn = 0;

            if (sessionsMenu->isVisible()) {
              sessionsMenu->hide();
            } else {
              int targetH = userShutdownPopupHeight();
              TQPoint mainPos = mapToGlobal(TQPoint(0, 0));
              int popY;
              if (ClassicXSettings::fullUserShutdownMenuHeight() ||
                  ClassicXSettings::sidebarUserOnTop()) {
                popY = mainPos.y();
              } else {
                popY = mainPos.y() + height() - targetH;
              }
              TQPoint pos(mapToGlobal((*it).rect.topRight()).x() - 6, popY);
              sessionsMenu->popup(pos);
            }
          }
          return;
        case 1: // Log Out
          if (logoutMenu) {
            if (sessionsMenu)
              sessionsMenu->hide();
            m_sidebarPopupHoverTimer->stop();
            m_pendingHoverBtn = -1;
            m_blockedHoverBtn = 1;

            if (logoutMenu->isVisible()) {
              logoutMenu->hide();
            } else {
              int targetH = userShutdownPopupHeight();
              TQPoint mainPos = mapToGlobal(TQPoint(0, 0));
              int popY;
              if (ClassicXSettings::fullUserShutdownMenuHeight()) {
                popY = mainPos.y();
              } else {
                popY = mainPos.y() + height() - targetH;
              }
              TQPoint pos(mapToGlobal((*it).rect.topRight()).x() - 6, popY);
              logoutMenu->popup(pos);
            }
          }
          return;
        case 2: // Settings
          if (sessionsMenu)
            sessionsMenu->hide();
          if (logoutMenu)
            logoutMenu->hide();

          if (e->button() == TQt::RightButton) {
            slotShowClassicXSettings();
          } else {
            TDEProcess *proc = new TDEProcess;
            connect(proc, TQT_SIGNAL(processExited(TDEProcess *)), proc,
                    TQT_SLOT(deleteLater()));
            *proc << "konqueror" << "--profile" << "ctrl_panel";
            if (!proc->start(TDEProcess::DontCare)) {
              delete proc;
            }
            hide();
          }
          return;
        case 3: // Pictures
          new KRun(KURL(TDEGlobalSettings::picturesPath()));
          hide();
          return;
        case 4: // Documents
          new KRun(KURL(TDEGlobalSettings::documentPath()));
          hide();
          return;
        case 5: // Downloads
          {
            TQString dlPath = TDEGlobalSettings::downloadPath();
            if (dlPath.isEmpty() || !TQDir(dlPath).exists()) {
              dlPath = TQDir::homeDirPath() + "/Downloads";
            }
            new KRun(KURL(dlPath));
            hide();
            return;
          }
        }
      }
    }

    // Clicked elsewhere in sidebar? Close all sidebar popups
    if (sessionsMenu)
      sessionsMenu->hide();
    if (logoutMenu)
      logoutMenu->hide();

    return; // eat other clicks in sidebar
  }
  TQMouseEvent newEvent = translateMouseEvent(e);
  PanelServiceMenu::mousePressEvent(&newEvent);
}

void PanelKMenu::mouseReleaseEvent(TQMouseEvent *e) {
  if (sideImageRect().contains(e->pos()))
    return;
  TQMouseEvent newEvent = translateMouseEvent(e);

  // Title toggles recent/most-used on left click only — do not steal
  // right-click.
  if (e->button() == TQt::LeftButton) {
    int id = idAt(newEvent.pos());
    if (id == serviceMenuEndId()) {
      slotToggleRecentMode();
      return;
    }
  }

  PanelServiceMenu::mouseReleaseEvent(&newEvent);
}

void PanelKMenu::slotToggleRecentMode() {
  RecentlyLaunchedApps::the().toggleRuntimeRecentVsOften();
  if (initialized()) {
    refreshRecentSection();
  } else {
    initialize();
  }
}

void PanelKMenu::mouseMoveEvent(TQMouseEvent *e) {
  // Ignore mouse movements during grace period (e.g. after search clear
  // resizing)
  if (blockMouseTimer && blockMouseTimer->isActive()) {
    return;
  }

  // Determine which sidebar popup menu is currently open (if any)
  // and its corresponding sidebar button index
  TQPopupMenu *activeSidePopup = 0;
  int activeSideBtn = -1;
  if (sessionsMenu && sessionsMenu->isVisible()) {
    activeSidePopup = sessionsMenu;
    activeSideBtn = 0;
  } else if (logoutMenu && logoutMenu->isVisible()) {
    activeSidePopup = logoutMenu;
    activeSideBtn = 1;
  }

  // Handle sidebar area: track hover over icon areas
  if (ClassicXSettings::useSidePixmap() && sideImageRect().contains(e->pos())) {
    // Close any open submenus (but NOT the main KMenu or our sidebar popups)
    int guard = 0;
    TQWidget *popup = TQApplication::activePopupWidget();
    while (popup && popup != this && popup != sessionsMenu &&
           popup != logoutMenu && guard < 10) {
      popup->hide();
      popup = TQApplication::activePopupWidget();
      ++guard;
    }
    setActiveItem(-1);

    int oldHover = m_hoveredSidebarBtn;
    int targetBtn = -1;

    int popupDelay = ClassicXSettings::sidebarHoverDelay();
    if (popupDelay < 100)
      popupDelay = 100;
    if (popupDelay > 1000)
      popupDelay = 1000;

    bool enableHover = ClassicXSettings::sidebarHoverMenu();

    const TQValueList<SidebarBtn> &btns = getActiveSidebarButtons();
    for (TQValueList<SidebarBtn>::ConstIterator it = btns.begin();
         it != btns.end(); ++it) {
      if ((*it).rect.contains(e->pos())) {
        targetBtn = (*it).id;
        break;
      }
    }

    if (targetBtn == 0) { // User
      if (sessionsMenu && sessionsMenu->isVisible()) {
        m_sidebarPopupHoverTimer->stop();
        m_pendingHoverBtn = -1;
      } else if (enableHover && m_blockedHoverBtn != 0) {
        if (m_pendingHoverBtn != 0) {
          m_pendingHoverBtn = 0;
          m_sidebarPopupHoverTimer->start(popupDelay, true);
        }
      }
    } else if (targetBtn == 1) { // Logout
      if (logoutMenu && logoutMenu->isVisible()) {
        m_sidebarPopupHoverTimer->stop();
        m_pendingHoverBtn = -1;
      } else if (enableHover && m_blockedHoverBtn != 1) {
        if (m_pendingHoverBtn != 1) {
          m_pendingHoverBtn = 1;
          m_sidebarPopupHoverTimer->start(popupDelay, true);
        }
      }
    } else { // Settings (2), Images (3), Documents (4), or empty sidebar area
             // (-1)
      m_sidebarPopupHoverTimer->stop();
      m_pendingHoverBtn = -1;
      if (targetBtn != -1) {
        if (sessionsMenu)
          sessionsMenu->hide();
        if (logoutMenu)
          logoutMenu->hide();
      }
    }

    if (m_blockedHoverBtn != -1 && targetBtn != m_blockedHoverBtn) {
      m_blockedHoverBtn = -1;
    }

    m_hoveredSidebarBtn = targetBtn;

    if (oldHover != m_hoveredSidebarBtn) {
      update(sideImageRect());
    }
    return;
  }

  // Mouse left sidebar area, clear hover timer and hover state
  m_sidebarPopupHoverTimer->stop();
  m_pendingHoverBtn = -1;
  m_blockedHoverBtn = -1;
  if (m_hoveredSidebarBtn != -1) {
    if (activeSidePopup) {
      m_hoveredSidebarBtn =
          activeSideBtn; // Keep active popup's button highlighted
    } else {
      m_hoveredSidebarBtn = -1;
      m_delayedHoverBtn = -1;
      popupCloseTimer->stop();
    }
    update(sideImageRect());
  }

  // If a sidebar popup is open, skip parent class handling entirely
  // (parent PanelServiceMenu manages submenus and could interfere with our
  // popup)
  if (activeSidePopup) {
    return;
  }

  TQMouseEvent newEvent = translateMouseEvent(e);
  PanelServiceMenu::mouseMoveEvent(&newEvent);

  // If we are in main menu area and an item is selected, ensure sidebar popups
  // are closed
  if (idAt(e->pos()) != -1) {
    if (sessionsMenu)
      sessionsMenu->hide();
    if (logoutMenu)
      logoutMenu->hide();
  }

  // Hovering results must not leave the query field unfocused. TQt3's
  // QPopupMenu::setActiveItem() steals focus on mouse move.
  if (m_inFlatSearchMode && searchEdit && searchEdit->isVisible() &&
      TQApplication::activePopupWidget() == this && !searchEdit->hasFocus()) {
    searchEdit->setFocus();
  }
}

static TQString toLowerAndUnaccent(const TQString &input) {
  uint len = input.length();
  if (len == 0) return TQString::null;

  TQString res;
  res.setLength(len);

  const TQChar *in = input.unicode();
  TQChar *out = const_cast<TQChar *>(res.unicode());

  for (uint i = 0; i < len; ++i) {
    ushort uc = in[i].unicode();
    switch (uc) {
    case 0x00E0:
    case 0x00E2:
    case 0x00E4:
    case 0x00C0:
    case 0x00C2:
    case 0x00C4:
      out[i] = 'a';
      break;
    case 0x00E8:
    case 0x00E9:
    case 0x00EA:
    case 0x00EB:
    case 0x00C8:
    case 0x00C9:
    case 0x00CA:
    case 0x00CB:
      out[i] = 'e';
      break;
    case 0x00EE:
    case 0x00EF:
    case 0x00CE:
    case 0x00CF:
      out[i] = 'i';
      break;
    case 0x00F4:
    case 0x00F6:
    case 0x00D4:
    case 0x00D6:
      out[i] = 'o';
      break;
    case 0x00F9:
    case 0x00FB:
    case 0x00FC:
    case 0x00D9:
    case 0x00DB:
    case 0x00DC:
      out[i] = 'u';
      break;
    case 0x00E7:
    case 0x00C7:
      out[i] = 'c';
      break;
    default:
      out[i] = in[i].lower();
      break;
    }
  }
  return res;
}

// Build a flat cache of all KService entries from KSycoca.
// Called once on first search, then reused for all subsequent keystrokes.
// O(N) complexity, N = total services. Avoids repeated tree traversal.
void PanelKMenu::buildServiceCache() {
  m_cachedServices.clear();
  KServiceGroup::Ptr root = KServiceGroup::root();
  if (!root) {
    m_servicesCached = true;
    return;
  }

  // H19: BFS with visited + max depth — malformed sycoca must not freeze
  // kicker.
  const int kMaxDepth = 32;
  TQValueList<KServiceGroup::Ptr> stack;
  TQValueList<int> depths;
  TQMap<TQString, bool> visited;
  stack.append(root);
  depths.append(0);
  while (!stack.isEmpty()) {
    KServiceGroup::Ptr group = stack.front();
    stack.pop_front();
    int depth = depths.front();
    depths.pop_front();
    if (!group || depth > kMaxDepth)
      continue;

    TQString rp = group->relPath();
    if (visited.contains(rp))
      continue;
    visited.insert(rp, true);

    if (group->noDisplay() || rp.startsWith("q4os-hdn") || rp.startsWith(".")) {
      continue;
    }

    KServiceGroup::List list =
        group->entries(true, true, true,
                       ClassicXSettings::menuEntryFormat() ==
                           ClassicXSettings::NameAndDescription);
    for (KServiceGroup::List::ConstIterator it = list.begin(); it != list.end();
         ++it) {
      KSycocaEntry *e = *it;
      if (e && e->isType(KST_KService)) {
        KService::Ptr s(static_cast<KService *>(e));
        if (s && !s->noDisplay()) {
          SearchIndexEntry entry;
          entry.service = s;
          entry.nameLower = toLowerAndUnaccent(s->name());
          entry.genericNameLower = toLowerAndUnaccent(s->genericName());
          static const TQRegExp wordSplitRx("[\\s_\\-\\/]+");
          entry.nameWords = TQStringList::split(wordSplitRx, entry.nameLower);
          entry.genericNameWords = entry.genericNameLower.isEmpty()
                                       ? TQStringList()
                                       : TQStringList::split(wordSplitRx, entry.genericNameLower);
          m_cachedServices.append(entry);
          if (!rp.isEmpty() && rp != "/") {
            TQString newPath = rp;
            TQString curSid = s->storageId();
            if (!s_serviceRelPaths.contains(curSid) ||
                newPath.length() > s_serviceRelPaths[curSid].length()) {
              s_serviceRelPaths[curSid] = newPath;
              s_serviceRelPaths[s->desktopEntryPath()] = newPath;
              s_serviceRelPaths[s->menuId()] = newPath;
            }
          }
        }
      } else if (e && e->isType(KST_KServiceGroup)) {
        stack.append(KServiceGroup::Ptr(static_cast<KServiceGroup *>(e)));
        depths.append(depth + 1);
      }
    }
  }
  m_servicesCached = true;
}

struct FuzzyCandidate {
  KService::Ptr service;
  int dist;
};

static int computeMinWordDistance(const TQString &termLower, const TQString &word,
                                   bool requireFirstLetterMatch, int maxDistLimit) {
  int len1 = termLower.length();
  int len2 = word.length();
  if (len1 == 0) return len2;
  if (len2 == 0) return len1;

  const int maxLen = 64;
  if (len1 > maxLen || len2 > maxLen) return 999;

  const TQChar *u1 = termLower.unicode();
  const TQChar *u2 = word.unicode();

  if (requireFirstLetterMatch && u1[0] != u2[0]) {
    return 999;
  }

  int d[3][65];
  for (int j = 0; j <= len2; ++j) {
    d[0][j] = j;
  }

  for (int i = 1; i <= len1; ++i) {
    int curr = i % 3;
    int prev = (i + 2) % 3;
    int prev2 = (i + 1) % 3;

    d[curr][0] = i;
    int minRowDist = i;

    for (int j = 1; j <= len2; ++j) {
      int cost = (u1[i - 1] == u2[j - 1]) ? 0 : 1;
      int dist = TQMIN(d[prev][j] + 1,
                       TQMIN(d[curr][j - 1] + 1, d[prev][j - 1] + cost));

      if (i > 1 && j > 1 && u1[i - 1] == u2[j - 2] && u1[i - 2] == u2[j - 1]) {
        dist = TQMIN(dist, d[prev2][j - 2] + cost);
      }
      d[curr][j] = dist;
      if (dist < minRowDist) {
        minRowDist = dist;
      }
    }

    if (minRowDist > maxDistLimit) {
      return 999;
    }
  }

  int lastRow = len1 % 3;
  int minD = d[lastRow][len2];

  int minK = TQMAX(2, len1 - 1);
  int maxK = TQMIN(len2, len1 + 2);
  for (int k = minK; k <= maxK; ++k) {
    int dPrefix = d[lastRow][k];
    if (dPrefix < minD) {
      minD = dPrefix;
    }
  }

  return minD;
}

static int getServiceRecencyRank(const KService::Ptr &s,
                                 const TQStringList &rList) {
  if (!s || rList.isEmpty())
    return 999999;
  TQString desktopPath = s->desktopEntryPath();
  TQString storageId = s->storageId();
  TQString menuId = s->menuId();

  int idx = 0;
  for (TQStringList::ConstIterator it = rList.begin(); it != rList.end();
       ++it, ++idx) {
    const TQString &rPath = *it;
    if ((!desktopPath.isEmpty() && rPath == desktopPath) ||
        (!storageId.isEmpty() && rPath == storageId) ||
        (!menuId.isEmpty() && rPath == menuId)) {
      return idx;
    }
  }
  return 999999;
}

struct RankedMatch {
  KService::Ptr service;
  int rank;
};

static void sortMatchesByRecency(TQValueList<KService::Ptr> &matchesList,
                                 const TQStringList &rList) {
  if (matchesList.count() <= 1 || rList.isEmpty())
    return;

  TQValueVector<RankedMatch> recentMatches;
  TQValueList<KService::Ptr> otherMatches;

  for (TQValueList<KService::Ptr>::Iterator it = matchesList.begin();
       it != matchesList.end(); ++it) {
    int rank = getServiceRecencyRank(*it, rList);
    if (rank < 999999) {
      RankedMatch rm;
      rm.service = *it;
      rm.rank = rank;
      recentMatches.append(rm);
    } else {
      otherMatches.append(*it);
    }
  }

  if (recentMatches.count() > 1) {
    for (unsigned int i = 1; i < recentMatches.count(); ++i) {
      RankedMatch key = recentMatches[i];
      int j = (int)i - 1;
      while (j >= 0 && recentMatches[j].rank > key.rank) {
        recentMatches[j + 1] = recentMatches[j];
        j--;
      }
      recentMatches[j + 1] = key;
    }
  }

  matchesList.clear();
  for (unsigned int i = 0; i < recentMatches.count(); ++i) {
    matchesList.append(recentMatches[i].service);
  }
  for (TQValueList<KService::Ptr>::Iterator it = otherMatches.begin();
       it != otherMatches.end(); ++it) {
    matchesList.append(*it);
  }
}

// Search items use IDs above the tree/recents range (4242–5242+) and
// below client menus (10000+). Hidden tree items keep their original IDs.
static const int searchItemIdBase = 6000;

// Keep a live popup inside the work area. TQPopupMenu::exec() ends if the
// window is mapped fully off-screen (typical with a top bar + "open upward"
// math). popupPosition() does not clamp Y for Up/Down.
static TQPoint clampPopupToWorkArea(const TQWidget *popup, TQPoint p)
{
  const TQRect ag =
      TQApplication::desktop()->availableGeometry(const_cast<TQWidget *>(popup));
  const int h = popup->height();
  const int w = popup->width();
  if (p.y() + h > ag.bottom() + 1)
    p.setY(ag.bottom() - h + 1);
  if (p.y() < ag.top())
    p.setY(ag.top());
  if (p.x() + w > ag.right() + 1)
    p.setX(ag.right() - w + 1);
  if (p.x() < ag.left())
    p.setX(ag.left());
  return p;
}

static TQPoint placePopupAgainstPanel(TQWidget *popup, ClassicXButton *btn)
{
  TQPoint p = popup->pos();
  if (btn && btn->topLevelWidget()) {
    TQWidget *panel = btn->topLevelWidget();
    if (ClassicXSettings::menuCentered()) {
      TQDesktopWidget *desktop = TQApplication::desktop();
      int scrNum = desktop->screenNumber(popup);
      TQRect screen = desktop->screenGeometry(scrNum);
      TQRect ag = desktop->availableGeometry(scrNum);

      int x = screen.left() + (screen.width() - popup->width()) / 2;
      int y;
      if (btn->popupDirection() == KPanelApplet::Up) {
        y = panel->y() - popup->height();
      } else {
        y = ag.bottom() - popup->height() + 1;
      }
      p = TQPoint(x, y);
    } else {
      switch (btn->popupDirection()) {
      case KPanelApplet::Up:
        p.setY(panel->y() - popup->height());
        break;
      case KPanelApplet::Down:
        p.setY(panel->y() + panel->height());
        break;
      case KPanelApplet::Left:
        p.setX(panel->x() - popup->width());
        break;
      case KPanelApplet::Right:
        p.setX(panel->x() + panel->width());
        break;
      }
    }
  }
  return clampPopupToWorkArea(popup, p);
}

void PanelKMenu::stashTreeForSearch()
{
  if (m_treeStashed)
    return;
  m_stashedTreeIds.clear();
  const int n = count();
  for (int i = 0; i < n; ++i) {
    const int id = idAt(i);
    if (id == searchLineID)
      continue;
    if (!isItemVisible(id))
      continue;
    setItemVisible(id, false);
    m_stashedTreeIds.append(id);
  }
  m_treeStashed = true;
}

void PanelKMenu::clearSearchItems()
{
  for (TQValueList<int>::ConstIterator it = m_searchItemIds.begin();
       it != m_searchItemIds.end(); ++it) {
    entryMap_.remove(*it);
    removeItem(*it);
  }
  m_searchItemIds.clear();
}

void PanelKMenu::restoreStashedTree()
{
  // Unhide the tree first. Clearing search items while the tree is still
  // hidden collapses a visible popup to ~0 height; Qt then pins it to the
  // bottom of the screen.
  for (TQValueList<int>::ConstIterator it = m_stashedTreeIds.begin();
       it != m_stashedTreeIds.end(); ++it) {
    setItemVisible(*it, true);
  }
  m_stashedTreeIds.clear();
  clearSearchItems();
  m_treeStashed = false;

  // Never removeItem(searchLineID): Qt3 deletes the custom widget and
  // type-to-search dies until the next full initialize().
  if (!ClassicXSettings::alwaysShowSearchBar()) {
    if (indexOf(searchLineID) >= 0)
      setItemVisible(searchLineID, false);
    if (searchEdit) {
      TQHBox *hbox = dynamic_cast<TQHBox *>(searchEdit->parent());
      if (hbox)
        hbox->hide();
    }
  }
}

void PanelKMenu::updateSearchEditPalette()
{
  if (!searchEdit)
    return;
  TQPalette editPal = searchEdit->palette();
  editPal.setColor(TQColorGroup::Base, KickerLib::getMenuTextBgColor());
  const TQColor textCol = KickerLib::getMenuSearchTextColor();
  editPal.setColor(TQColorGroup::Text, textCol);
  editPal.setColor(TQColorGroup::Foreground, textCol);
  searchEdit->setPalette(editPal);
}

void PanelKMenu::ensureSearchEdit()
{
  if (searchEdit)
    return;
  if (!ClassicXSettings::useSearchBar())
    return;

  TQHBox *hbox = new TQHBox(this);
  hbox->setMargin(0);
  hbox->setSpacing(0);
  TDEToolBarButton *clearButton =
      new TDEToolBarButton("locationbar_erase", 0, hbox);
  TQString placeholder = i18n("Type here to search...");
  searchEdit = new ClassicXSearchLineEdit(hbox, placeholder);
  TQWidget *rightMargin = new TQWidget(hbox);
  rightMargin->setFixedWidth(8);
  updateSearchEditPalette();
  hbox->setFocusProxy(searchEdit);
  hbox->setSpacing(3);
  connect(clearButton, TQT_SIGNAL(clicked()), searchEdit, TQT_SLOT(clear()));
  connect(this, TQT_SIGNAL(aboutToHide()), this, TQT_SLOT(slotClearSearch()));
  connect(searchEdit, TQT_SIGNAL(textChanged(const TQString &)), this,
          TQT_SLOT(slotUpdateSearch(const TQString &)));
  connect(searchEdit, TQT_SIGNAL(returnPressed()), this,
          TQT_SLOT(slotSearchReturnPressed()));
  searchEdit->installEventFilter(this);
}

void PanelKMenu::showSearchBarItem()
{
  ensureSearchEdit();
  if (!searchEdit)
    return;
  TQHBox *hbox = dynamic_cast<TQHBox *>(searchEdit->parent());
  if (!hbox)
    return;
  hbox->show();
  // If already a menu item, do NOT reparent — that tears the widget out
  // of the QMenuItem and the field vanishes on the next search.
  if (indexOf(searchLineID) < 0) {
    hbox->reparent(this, TQPoint(0, 0));
    insertItem(hbox, searchLineID);
  }
  setItemVisible(searchLineID, true);
}

void PanelKMenu::slotUpdateSearch(const TQString &searchString) {
  TQString term = searchString.stripWhiteSpace();

  TQStringList rList;
  RecentlyLaunchedApps::the().getRecentApps(rList);

  if (!m_inFlatSearchMode) {
    if (width() > 100) {
      m_widthBeforeSearch = width();
    }
    m_mainMenuPos = pos();
    m_mainMenuSize = size();
    m_savedRecentCount = rList.count();
  }
  m_inFlatSearchMode = true;
  m_lastHighlightedId = -1;

  // Freeze height before hiding the tree. A visible popup that collapses to
  // a few pixels is remapped by Qt (often to the bottom of the screen) or
  // unmapped if that sliver sits inside a top-panel strut — exec() then ends.
  const bool searchVis = isVisible();
  const int freezeH = searchVis ? height() : 0;
  const bool searchUpd = isUpdatesEnabled();
  if (searchVis) {
    setUpdatesEnabled(false);
    if (freezeH > 0)
      setMinimumHeight(freezeH);
  }

  if (m_inOpeningAnim) {
    m_inOpeningAnim = false;
    m_openingAnimTimer->stop();
    ClassicX::applyWindowOpacity(winId(), ClassicXSettings::classicKMenuOpacity());
    updateTopPicMask();
  }

  if (!m_treeStashed)
    stashTreeForSearch();
  else
    clearSearchItems();

  int index = 0;
  if (topPixHeight > 0) {
    int spacerId = insertItem(new TopSpacerMenuItem(topPixHeight), -1, 0);
    setItemEnabled(spacerId, false);
    m_searchItemIds.append(spacerId);
    index = 1;
  }

  int maxSearchLimit = ClassicXSettings::maxSearchResults();
  if (maxSearchLimit < 1)
    maxSearchLimit = 1;
  if (maxSearchLimit > 30)
    maxSearchLimit = 30;

  TQValueList<KService::Ptr> matches;
  int totalMatches = 0;
  bool isFuzzySuggestion = false;

  if (!term.isEmpty()) {
    // Build service cache on first search of this session
    if (!m_servicesCached) {
      buildServiceCache();
    }

    TQString termLower = toLowerAndUnaccent(term);

    // Filter from cached list instead of re-traversing KSycoca
    for (TQValueList<SearchIndexEntry>::ConstIterator it =
             m_cachedServices.begin();
         it != m_cachedServices.end(); ++it) {
      const KService::Ptr &s = (*it).service;
      if (s && ((*it).nameLower.find(termLower) != -1 ||
                (!(*it).genericNameLower.isEmpty() &&
                 (*it).genericNameLower.find(termLower) != -1))) {
        totalMatches++;
        if (matches.count() < maxSearchLimit) {
          matches.append(s);
        }
        if (totalMatches > 50)
          break;
      }
    }

    // Prioritize recently launched apps within exact search matches
    sortMatchesByRecency(matches, rList);

    // Fuzzy suggestion if no exact matches and term is at least 3 characters
    // long
    if (matches.isEmpty() && termLower.length() >= 3) {
      int maxDist = (termLower.length() <= 4) ? 1 : 2;

      TQValueList<FuzzyCandidate> fuzzyList;

      for (TQValueList<SearchIndexEntry>::ConstIterator it =
               m_cachedServices.begin();
           it != m_cachedServices.end(); ++it) {
        const KService::Ptr &s = (*it).service;
        if (!s)
          continue;

        const TQStringList &nameWords = (*it).nameWords;
        const TQStringList &genWords = (*it).genericNameWords;

        int minD = 999;
        int termLen = termLower.length();

        // 1. Evaluate app name words (flexible distance matching)
        for (TQStringList::ConstIterator wIt = nameWords.begin();
             wIt != nameWords.end(); ++wIt) {
          const TQString &word = *wIt;
          if (word.length() < 2) continue;

          int d = computeMinWordDistance(termLower, word, false, maxDist);
          if (d < minD) minD = d;
        }

        // 2. Evaluate generic description words (stricter: first-letter match required & dist <= 1)
        for (TQStringList::ConstIterator wIt = genWords.begin();
             wIt != genWords.end(); ++wIt) {
          const TQString &word = *wIt;
          if (word.length() < 2) continue;

          int d = computeMinWordDistance(termLower, word, true, 1);
          if (d <= 1 && d < minD) minD = d;
        }

        if (minD <= maxDist) {
          FuzzyCandidate fc;
          fc.service = s;
          fc.dist = minD;
          fuzzyList.append(fc);
        }
      }

      if (!fuzzyList.isEmpty()) {
        isFuzzySuggestion = true;
        // Add fuzzy candidates (distance 1 before distance 2), prioritizing
        // recently launched apps within each distance level
        for (int dStep = 1; dStep <= maxDist; ++dStep) {
          TQValueList<KService::Ptr> stepMatches;
          for (TQValueList<FuzzyCandidate>::ConstIterator fIt =
                   fuzzyList.begin();
               fIt != fuzzyList.end(); ++fIt) {
            if ((*fIt).dist == dStep) {
              stepMatches.append((*fIt).service);
            }
          }
          if (!stepMatches.isEmpty()) {
            sortMatchesByRecency(stepMatches, rList);
            for (TQValueList<KService::Ptr>::Iterator sIt = stepMatches.begin();
                 sIt != stepMatches.end(); ++sIt) {
              if (matches.count() < maxSearchLimit) {
                matches.append(*sIt);
              }
            }
          }
        }
        totalMatches = matches.count();
      }
    }
  }

  TQString titleString;
  TQString suffixString;

  if (term.isEmpty()) {
    titleString = i18n("Type to search...");
  } else if (isFuzzySuggestion) {
    titleString = i18n("No exact results found.");
    suffixString = i18n(" Did you mean:");
  } else if (totalMatches == 0) {
    titleString = i18n("No Results Found");
  } else if (totalMatches == 1) {
    titleString = i18n("Search Result: ");
    suffixString = i18n("[ENTER] To Launch");
  } else if (totalMatches > 50) {
    titleString = i18n("Search Results (50+)");
  } else {
    titleString = i18n("Search Results (%1)").arg(totalMatches);
  }

  PopupMenuTitle *titleItem = new PopupMenuTitle(titleString, font());
  if (!suffixString.isEmpty()) {
    titleItem->setSuffix(suffixString);
  }
  int titleId = insertItem(titleItem, -1, index++);
  setItemEnabled(titleId, false);
  m_searchItemIds.append(titleId);

  int id = searchItemIdBase;
  int resultCount = 0;
  m_searchResultsServices = matches;

  if (!matches.isEmpty()) {
    for (TQValueList<KService::Ptr>::Iterator it = matches.begin();
         it != matches.end(); ++it) {
      if (*it) {
        insertMenuItem(*it, id, index++);
        m_searchItemIds.append(id);
        id++;
        resultCount++;
      }
    }
  }
  // Pad remaining slots so search height stays constant (maxSearchLimit rows).
  // Pads must match result row metrics: same S×S icon, or no icon if apps have none.
  int targetSlots = maxSearchLimit;
  if (targetSlots < 0)
    targetSlots = 0;
  if (resultCount < targetSlots) {
    const bool usePadIcon = ClassicXSettings::showAppIcons();
    TQIconSet padIcon;
    if (usePadIcon)
      padIcon = KickerLib::treeIconPadIconSet();
    for (int pad = resultCount; pad < targetSlots; ++pad) {
      int padId = usePadIcon
                      ? insertItem(padIcon, TQString::fromLatin1(" "), -1, index++)
                      : insertItem(TQString::fromLatin1(" "), -1, index++);
      setItemEnabled(padId, false);
      m_searchItemIds.append(padId);
    }
  }
  m_searchItemIds.append(insertSeparator(index++));
  showSearchBarItem();

  int baseW = (m_widthBeforeSearch > 100)
                  ? m_widthBeforeSearch
                  : ((m_mainMenuSize.isValid() && m_mainMenuSize.width() > 100)
                         ? m_mainMenuSize.width()
                         : sizeHint().width());
  int minW = ClassicXSettings::menuMinWidth();
  int searchW = (minW > 0) ? kMax(baseW, minW) : (int)(baseW * 1.5);
  setFixedWidth(searchW);

  if (searchVis) {
    setMinimumHeight(0);
    setMaximumHeight(30000);
    adjustSize();
    move(placePopupAgainstPanel(this, m_appletButton));
    setUpdatesEnabled(searchUpd);
  }

  if (!isFuzzySuggestion && totalMatches == 1 && !matches.isEmpty()) {
    m_singleMatch = matches.first();
  } else {
    m_singleMatch = 0;
  }

  if (searchEdit) {
    searchEdit->setHoldFocus(true);
    searchEdit->setFocus();
  }
  updateTopPicMask();
}

void PanelKMenu::slotSearchItemHighlighted(int id)
{
  m_lastHighlightedId = id;
}

void PanelKMenu::slotOpeningAnimStep() {
  if (!m_inOpeningAnim) {
    m_openingAnimTimer->stop();
    return;
  }

  m_openingAnimStep++;

  float t = (float)m_openingAnimStep / (float)m_totalOpeningAnimSteps;
  if (t > 1.0f) t = 1.0f;
  float factor = 1.0f - (1.0f - t) * (1.0f - t) * (1.0f - t);

  int targetOpacity = ClassicXSettings::classicKMenuOpacity();
  int curOpacity = (int)((float)targetOpacity * (0.20f + 0.80f * factor) + 0.5f);
  if (curOpacity < 1) curOpacity = 1;
  if (curOpacity > 100) curOpacity = 100;
  ClassicX::applyWindowOpacity(winId(), curOpacity);

  repaint(false);
}

void PanelKMenu::slotRestoreStashAfterHide()
{
  if (isVisible() || !m_treeStashed)
    return;
  restoreStashedTree();
  if (m_mainMenuSize.isValid() && m_mainMenuSize.height() > 0)
    resize(m_mainMenuSize.width(), m_mainMenuSize.height());
}

void PanelKMenu::slotSearchReturnPressed() {
  if (!m_inFlatSearchMode)
    return;

  int id = m_lastHighlightedId;
  if (id >= 0 && isItemEnabled(id) && entryMap_.contains(id)) {
    KSycocaEntry *e = entryMap_[id];
    if (e && e->isType(KST_KService)) {
      PanelServiceMenu::slotExec(id);
      hide();
      return;
    }
  }

  KService::Ptr match = m_singleMatch;
  if (match) {
    m_singleMatch = 0;
    updateRecentlyUsedApps(match);
    TDEApplication::startServiceByDesktopPath(
        match->desktopEntryPath(), TQStringList(), 0, 0, 0, "", true);
    TQTimer::singleShot(0, this, TQT_SLOT(hide()));
  }
}

void PanelKMenu::slotClearSearch() {
  if (popupMenu_ && popupMenu_->isVisible()) {
    return;
  }

  if (searchEdit) {
    searchEdit->setHoldFocus(false);
    searchEdit->blockSignals(true);
    searchEdit->clear();
    searchEdit->blockSignals(false);
    searchEdit->clearFocus();
  }

  if (m_inFlatSearchMode) {
    m_inFlatSearchMode = false;
    // Tree is only hidden (stash), not destroyed — same as a normal close.

    // Unlock fixed width/height constraints
    setMinimumSize(0, 0);
    setMaximumSize(30000, 30000);
    setSizePolicy(
        TQSizePolicy(TQSizePolicy::Preferred, TQSizePolicy::Preferred));

    if (m_widthBeforeSearch > 0) {
      setMinimumWidth(m_widthBeforeSearch);
      setMaximumWidth(30000);
      m_widthBeforeSearch = 0;
    }
  }
}

void PanelKMenu::slotRestoreMainMenu() {
  // Copy before restore: unhiding/clearing can trigger showEvent →
  // slotCaptureMainMenuGeometry and overwrite the saved main-menu rect.
  const TQSize savedSize = m_mainMenuSize;
  const int savedRecent = m_savedRecentCount;

  slotClearSearch();

  if (isVisible()) {
    const bool up = isUpdatesEnabled();
    setUpdatesEnabled(false);
    if (m_treeStashed) {
      restoreStashedTree();
    } else if (!initialized()) {
      clear();
      initialize();
    }
    setUpdatesEnabled(up);
    if (savedSize.isValid() && savedSize.height() > 0) {
      TQStringList curList;
      RecentlyLaunchedApps::the().getRecentApps(curList);
      int curCount = curList.count();

      int finalH = savedSize.height();

      if (curCount != savedRecent && savedRecent >= 0) {
        int itemDelta = curCount - savedRecent;
        int rowH = fontMetrics().height();
        if (ClassicXSettings::showAppIcons()) {
          int iconH = KickerLib::treeIconPixelSize();
          if (iconH > rowH)
            rowH = iconH;
        }
        rowH += 4; // Qt3 menu-item chrome (20px icon → 24px row)
        finalH += itemDelta * rowH;
        m_savedRecentCount = curCount;
      }

      m_mainMenuSize = TQSize(savedSize.width(), finalH);
      resize(savedSize.width(), finalH);
      const TQPoint p = placePopupAgainstPanel(this, m_appletButton);
      m_mainMenuPos = p;
      move(p);
    }
    updateTopPicMask();
    setFocus();
  }
}

void PanelKMenu::slotReinitialize() {
  if (isVisible()) {
    hide();
  }
  m_inFlatSearchMode = false;
  m_widthBeforeSearch = 0;
  m_baseContentFloorWidth = 0;
  m_mainMenuSize = TQSize();
  m_mainMenuPos = TQPoint();
  m_savedRecentCount = -1;
  slotClear();
  // User/Shutdown popups keep TQt3 size caches (maxPMWidth, setFixedHeight)
  // across settings OK. Recreate them like a fresh applet start.
  resetSidebarPopups();
  m_lastSidebarHeight = -1;
  loadSidePixmap();
  setInitialized(false);
  setMinimumSize(0, 0);
  setMaximumSize(30000, 30000);
  clear();
  initialize();
  ensureLogoutMenu();
  resize(sizeHint().width(), sizeHint().height());
  adjustSize();
  updateGeometry();
}

void PanelKMenu::slotClear() {
  m_treeStashed = false;
  m_stashedTreeIds.clear();
  m_searchItemIds.clear();
  s_serviceRelPaths.clear();
  m_servicesCached = false;
  m_cachedServices.clear();
  m_cachedTopPicTitle = TQString::null;
  m_lastTopPicMinute = -1;
  m_lastSidebarHeight = -1;
  m_lastSidebarWidth = -1;
  m_lastSidebarIconSize = -1;
  m_lastSidebarAlign = -1;
  PanelServiceMenu::slotClear();
}

void PanelKMenu::slotClearOnClose() {
  s_serviceRelPaths.clear();
  m_servicesCached = false;
  m_cachedServices.clear();
  PanelServiceMenu::slotClearOnClose();
}

void PanelKMenu::focusInEvent(TQFocusEvent *e) {
  // During search, hovering items makes TQt3 give focus to the popup.
  // Send it back to the field so the query is not reset / replaced.
  if (m_inFlatSearchMode && searchEdit && searchEdit->isVisible()) {
    searchEdit->setFocus();
    return;
  }
  PanelServiceMenu::focusInEvent(e);
}

void PanelKMenu::keyPressEvent(TQKeyEvent *e) {
  // Navigation keys always passthrough to standard menu handling
  if (e->key() == TQt::Key_Up || e->key() == TQt::Key_Down ||
      e->key() == TQt::Key_Left || e->key() == TQt::Key_Right ||
      e->key() == TQt::Key_Return || e->key() == TQt::Key_Enter) {
    KPanelMenu::keyPressEvent(e);
    return;
  }

  // Type-to-Search: Escape exits search mode and restores main menu if active
  if (e->key() == TQt::Key_Escape) {
    if (m_inFlatSearchMode) {
      slotRestoreMainMenu();
      return;
    }
    KPanelMenu::keyPressEvent(e);
    return;
  }

  // '/' Shortcut (Legacy support) or Type-to-Search (Alphanumeric)
  bool isSlash = (e->text() == "/");
  bool isTypeToSearch = (!e->text().isEmpty() && e->text().length() == 1 &&
                         e->text()[0].isPrint() &&
                         !(e->state() & (TQt::ControlButton | TQt::AltButton)));

  if (isSlash || isTypeToSearch) {
    ensureSearchEdit();
    if (!searchEdit) {
      KPanelMenu::keyPressEvent(e);
      return;
    }
    if (!m_inFlatSearchMode) {
      if (width() > 100) {
        m_widthBeforeSearch = width();
      }
      m_mainMenuPos = pos();
      m_mainMenuSize = size();
      TQStringList rList;
      RecentlyLaunchedApps::the().getRecentApps(rList);
      m_savedRecentCount = rList.count();
      m_inFlatSearchMode = true;

      if (sessionsMenu)
        sessionsMenu->hide();
      if (logoutMenu)
        logoutMenu->hide();
      if (treeUserMenu)
        treeUserMenu->hide();
      if (treeShutdownMenu)
        treeShutdownMenu->hide();

      showSearchBarItem();
    }

    searchEdit->setHoldFocus(true);
    searchEdit->setFocus();
    if (isTypeToSearch) {
      // After a hover focus-steal, this path used to setText() and wipe
      // the query. Append at the caret instead.
      if (!searchEdit->text().isEmpty()) {
        searchEdit->deselect();
        searchEdit->setCursorPosition(searchEdit->text().length());
        searchEdit->insert(e->text());
      } else {
        searchEdit->setText(e->text());
      }
    }
    return;
  }

  KPanelMenu::keyPressEvent(e);
}

void PanelKMenu::slotSidebarPopupHoverTimeout() {
  if (m_pendingHoverBtn == -1)
    return;

  TQValueList<SidebarBtn> btns = getActiveSidebarButtons();
  TQRect targetRect;
  bool found = false;

  for (TQValueList<SidebarBtn>::ConstIterator it = btns.begin();
       it != btns.end(); ++it) {
    if ((*it).id == m_pendingHoverBtn) {
      targetRect = (*it).rect;
      found = true;
      break;
    }
  }

  if (found) {
    if (m_pendingHoverBtn == 0) { // Switch User
      if (logoutMenu)
        logoutMenu->hide();
      ensureSessionsMenu();
      if (sessionsMenu) {
        int targetH = userShutdownPopupHeight();
        TQPoint mainPos = mapToGlobal(TQPoint(0, 0));
        int popY;
        if (ClassicXSettings::fullUserShutdownMenuHeight() ||
            ClassicXSettings::sidebarUserOnTop()) {
          popY = mainPos.y();
        } else {
          popY = mainPos.y() + height() - targetH;
        }
        TQPoint pos(mapToGlobal(targetRect.topRight()).x() - 6, popY);
        sessionsMenu->popup(pos);
      }
    } else if (m_pendingHoverBtn == 1) { // Log Out
      if (sessionsMenu)
        sessionsMenu->hide();
      if (logoutMenu) {
        int targetH = userShutdownPopupHeight();
        TQPoint mainPos = mapToGlobal(TQPoint(0, 0));
        int popY;
        if (ClassicXSettings::fullUserShutdownMenuHeight()) {
          popY = mainPos.y();
        } else {
          popY = mainPos.y() + height() - targetH;
        }
        TQPoint pos(mapToGlobal(targetRect.topRight()).x() - 6, popY);
        logoutMenu->popup(pos);
      }
    }
  }
  m_pendingHoverBtn = -1;
}

void PanelKMenu::slotClosePopupTimeout() {
  // Determine which sidebar popup menu is currently open
  TQPopupMenu *activeSidePopup = 0;
  int activeSideBtn = -1;
  if (sessionsMenu && sessionsMenu->isVisible()) {
    activeSidePopup = sessionsMenu;
    activeSideBtn = 0;
  } else if (logoutMenu && logoutMenu->isVisible()) {
    activeSidePopup = logoutMenu;
    activeSideBtn = 1;
  }

  if (activeSidePopup) {
    // Timeout expired: apply delayed hover switch and close popup
    if (m_delayedHoverBtn != -1 && m_delayedHoverBtn != activeSideBtn) {
      activeSidePopup->hide();
      m_hoveredSidebarBtn = m_delayedHoverBtn;
      m_delayedHoverBtn = -1;
      update(sideImageRect());
    } else if (m_hoveredSidebarBtn != -1 &&
               m_hoveredSidebarBtn != activeSideBtn) {
      // Fallback for direct hover check
      activeSidePopup->hide();
    }
  }
}

void PanelKMenu::slotOnShow() {
  if (m_treeStashed)
    restoreStashedTree();

  RecentlyLaunchedApps::the().syncRuntimeWithSettings();

  // Do not init(true) here: that clears the in-memory list and reloads
  // kickerrc. appLaunched() only flushes to disk after 2s, so a reopen
  // before that would drop the launch and rewrite the old list.
  if (!initialized()) {
    initialize();
    adjustSize();
  } else if (RecentlyLaunchedApps::the().m_bNeedToUpdate) {
    refreshRecentSection();
    RecentlyLaunchedApps::the().m_bNeedToUpdate = false;
    adjustSize();
  }

  ClassicX::applyWindowOpacity(winId(),
                               ClassicXSettings::classicKMenuOpacity());
}

void PanelKMenu::clearRecentSectionFromMenu() {
  // IDs used by createRecentMenuItems():
  //   top tile  = serviceMenuEndId() - 1
  //   title     = serviceMenuEndId()
  //   apps      = serviceMenuEndId() + 1 .. +N  (N <= 6, scan a safe margin)
  const int titleId = serviceMenuEndId();
  const int topId = serviceMenuEndId() - 1;

  if (indexOf(topId) >= 0)
    removeItem(topId);
  if (indexOf(titleId) >= 0)
    removeItem(titleId);

  for (int id = serviceMenuEndId() + 1; id <= serviceMenuEndId() + 12; ++id) {
    if (indexOf(id) >= 0) {
      removeItem(id);
      entryMap_.remove(id);
    }
  }

  // Separator that followed the recent block is now at start (if present).
  int startIdx = (topPixHeight > 0) ? 1 : 0;
  while (count() > startIdx) {
    TQMenuItem *mi = findItem(idAt(startIdx));
    if (mi && mi->isSeparator()) {
      removeItemAt(startIdx);
      continue;
    }
    break;
  }

  RecentlyLaunchedApps::the().m_nNumMenuItems = 0;
}

void PanelKMenu::refreshRecentSection() {
  clearRecentSectionFromMenu();
  createRecentMenuItems();
}

bool PanelKMenu::eventFilter(TQObject *o, TQEvent *e) {
  // Only restyle/opacity our own menus. Plain TDEPopupMenu context menus must
  // be left alone — setPalette/XOpacity during their Show was a crash
  // candidate.
  if (!m_inEventFilter && e && e->type() == TQEvent::Show && o &&
      o->isWidgetType()) {
    TQPopupMenu *menu = 0;
    if (o->inherits("PanelServiceMenu") || o == sessionsMenu ||
        o == logoutMenu || o == treeUserMenu || o == treeShutdownMenu) {
      menu = static_cast<TQPopupMenu *>(o);
    }
    if (menu) {
      m_inEventFilter = true;
      KickerLib::updateMenuPalette(menu);
      ClassicX::applyWindowOpacity(menu->winId(),
                                   ClassicXSettings::classicKMenuOpacity());
      m_inEventFilter = false;
    }
  }
  // Stop grace period timer if mouse enters the popup menu
  if ((o == sessionsMenu || o == logoutMenu) &&
      (e->type() == TQEvent::Enter || e->type() == TQEvent::MouseMove)) {
    if (popupCloseTimer->isActive()) {
      popupCloseTimer->stop();
    }
  }

  if (o == sessionsMenu && e->type() == TQEvent::MouseButtonRelease) {
    TQMouseEvent *me = static_cast<TQMouseEvent *>(e);
    if (me->button() == TQMouseEvent::LeftButton) {
      int id = sessionsMenu->idAt(me->pos());
      if (id == 97) { // User name title ID
        hide();
        TDEProcess *proc = new TDEProcess;
        connect(proc, TQT_SIGNAL(processExited(TDEProcess *)), proc,
                TQT_SLOT(deleteLater()));
        *proc << "tdecmshell" << "System/userconfig";
        if (!proc->start(TDEProcess::DontCare)) {
          delete proc;
        }
        return true;
      }
    }
  }

  if (o == logoutMenu && e->type() == TQEvent::MouseButtonRelease) {
    TQMouseEvent *me = static_cast<TQMouseEvent *>(e);
    if (me->button() == TQMouseEvent::LeftButton) {
      int id = logoutMenu->idAt(me->pos());
      if (id == 96) { // Shutdown title ID
        slotLogout();
        return true;
      }
    }
  }

  if (o == searchEdit && e->type() == TQEvent::FocusIn) {
    TQFocusEvent *fe = static_cast<TQFocusEvent *>(e);
    // Click/tab into the field: drop the highlighted item. Do not do this
    // on hover focus-restore (Other) — that would emit highlighted(-1)
    // and look like the query was reset.
    if (fe->reason() == TQFocusEvent::Mouse ||
        fe->reason() == TQFocusEvent::Tab) {
      setActiveItem(-1);
    }
  }

  return KPanelMenu::eventFilter(o, e);
}

void PanelKMenu::configChanged() {
  RecentlyLaunchedApps::the().m_bNeedToUpdate = false;
  RecentlyLaunchedApps::the().configChanged();

  if (!loadSidePixmap()) {
    sidePixmap = sideTilePixmap = TQPixmap();
  }
  loadTopPixmap();

  PanelServiceMenu::configChanged();
}

// create and fill "recent" section at first
void PanelKMenu::createRecentMenuItems() {
  RecentlyLaunchedApps::the().m_nNumMenuItems = 0;

  if (!ClassicXSettings::showRecentApps()) {
    return;
  }

  TQStringList RecentApps;
  RecentlyLaunchedApps::the().getRecentApps(RecentApps);

  if (RecentApps.count() > 0) {
    int nId = serviceMenuEndId() + 1;

    int maximumNum = ClassicXSettings::numRecentApps();
    if (maximumNum < 2)
      maximumNum = 2;
    if (maximumNum > 6)
      maximumNum = 6;

    int topBaseIndex = (topPixHeight > 0) ? 1 : 0;
    int nBaseIndex =
        topBaseIndex + 1; // title at topBaseIndex, items at topBaseIndex + 1

    // First pass: collect up to maximumNum valid, displayable, non-duplicate
    // services
    TQValueList<KService::Ptr> validServices;
    for (TQValueList<TQString>::ConstIterator it = RecentApps.begin();
         it != RecentApps.end() && (int)validServices.count() < maximumNum;
         ++it) {
      KService::Ptr s = KService::serviceByDesktopPath(*it);
      if (!s) {
        s = KService::serviceByStorageId(*it);
      }
      if (!s) {
        TQString name = *it;
        if (name.endsWith(".desktop")) {
          name.truncate(name.length() - 8);
        }
        s = KService::serviceByDesktopName(name);
      }
      if (s && !s->noDisplay() && !s->name().isEmpty() &&
          s->name().at(0) != '.') {
        bool duplicate = false;
        for (TQValueList<KService::Ptr>::ConstIterator vIt =
                 validServices.begin();
             vIt != validServices.end(); ++vIt) {
          if ((*vIt)->desktopEntryPath() == s->desktopEntryPath()) {
            duplicate = true;
            break;
          }
        }
        if (!duplicate) {
          validServices.append(s);
        }
      }
    }

    // Second pass: insert into menu (most frequent at top)
    if (validServices.count() > 0) {
      // Insert title
      int id = insertItem(
          new PopupMenuTitle(RecentlyLaunchedApps::the().caption(), font()),
          serviceMenuEndId(), topBaseIndex);
      setItemEnabled(id, false);

      int pos = 0;
      for (TQValueList<KService::Ptr>::Iterator sIt = validServices.begin();
           sIt != validServices.end(); ++sIt, ++pos) {
        TQString appName = (*sIt)->name();
        if (appName.length() > 28) {
          appName.truncate(25);
          appName += "...";
        }
        insertMenuItem(*sIt, nId++, nBaseIndex + pos, 0, TQString::null,
                       appName);
      }
      RecentlyLaunchedApps::the().m_nNumMenuItems = validServices.count();

      insertSeparator(nBaseIndex + (int)validServices.count());
    }
  }
}

void PanelKMenu::clearSubmenus() {
  // we don't need to delete these on the way out since the libloader
  // handles them for us
  if (TQApplication::closingDown()) {
    // Still drop bookmarkMenu: it is not a libloader plugin and would UAF
    // once ~PanelServiceMenu deletes bookmarkParent from subMenus.
    delete bookmarkMenu;
    bookmarkMenu = 0;
    // Members may still be destroyed as TQObject children; never leave stale
    // non-null.
    treeUserMenu = 0;
    treeShutdownMenu = 0;
    return;
  }

  for (PopupMenuList::const_iterator it = dynamicSubMenus.constBegin();
       it != dynamicSubMenus.constEnd(); ++it) {
    delete *it;
  }
  dynamicSubMenus.clear();

  // KBookmarkMenu references bookmarkParent which lives in subMenus — destroy
  // controller first.
  delete bookmarkMenu;
  bookmarkMenu = 0;

  PanelServiceMenu::clearSubmenus();

  // treeUserMenu / treeShutdownMenu were appended to subMenus and are now
  // deleted.
  treeUserMenu = 0;
  treeShutdownMenu = 0;
}

void PanelKMenu::updateRecent() {
  RecentlyLaunchedApps::the().m_bNeedToUpdate = false;
  setInitialized(false);
  initialize();
  adjustSize();
}

void PanelKMenu::clearRecentMenuItems() {
  RecentlyLaunchedApps::the().clearRecentApps();
  RecentlyLaunchedApps::the().save();
  RecentlyLaunchedApps::the().m_bNeedToUpdate = true;
  updateRecent();
}
