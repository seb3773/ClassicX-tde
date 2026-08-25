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

#ifndef __k_mnu_h__
#define __k_mnu_h__

#include <dcopobject.h>
#include <tqintdict.h>
#include <tqpixmap.h>
#include <tqtimer.h>
#include <tqguardedptr.h>
#include <kpanelapplet.h>

class ClassicXButton;

#ifndef TQT_SIGNAL
#define TQT_SIGNAL TQ_SIGNAL
#endif
#ifndef TQT_SLOT
#define TQT_SLOT TQ_SLOT
#endif

#include "service_mnu.h"

#include "classicx_searchlineedit.h"

class KickerClientMenu;
class KBookmarkMenu;
class TDEActionCollection;
class KBookmarkOwner;
class Panel;
class TDEAccel;

class PanelKMenu : public PanelServiceMenu, public DCOPObject
{
    TQ_OBJECT
    K_DCOP

k_dcop:
    void slotServiceStartedByStorageId(TQString starter, TQString desktopPath);
    void hideMenu();

public:
    PanelKMenu();
    ~PanelKMenu();

    void setButton(ClassicXButton *btn) { m_appletButton = btn; }
    ClassicXButton *appletButton() const { return m_appletButton; }

    int insertClientMenu(KickerClientMenu *p);
    void removeClientMenu(int id);

    virtual TQSize sizeHint() const;
    virtual void setMinimumSize(const TQSize &);
    virtual void setMaximumSize(const TQSize &);
    virtual void setMinimumSize(int, int);
    virtual void setMaximumSize(int, int);
    virtual void showMenu();
    virtual bool inFlatSearchMode() const { return m_inFlatSearchMode; }
    void clearRecentMenuItems();
    TQPixmap renderMenuSnapshot();

public slots:
    void slotContextMenu(int);
    virtual void initialize();
    void slotShowClassicXSettings();

    //### KDE4: workaround for Qt bug, remove later
    virtual void resize(int width, int height);

protected slots:
    void slotControlCenter();
    void slotLock();
    void slotLogout();
    void slotShutdown();
    void slotReboot();
    void slotSuspend(int);
    void slotPopulateSessions();
    void slotPopulateLogout();
    void slotSessionActivated( int );
    void slotSaveSession();
    void slotInitPowerSystem();

private:
    ClassicXButton *m_appletButton;
    int m_baseContentFloorWidth;
    int m_widthBeforeSearch;
    bool m_canShutdown;
    bool m_canReboot; // Not used strictly but good for consistency? Setup uses canPowerOff for both.
    bool m_canSuspend;
    bool m_canHibernate;
    bool m_canHybridSuspend;
    bool m_canFreeze;
    bool m_powerSystemInitialized;
protected slots:
    void slotRunCommand();
    void slotEditUserContact();
    void slotUpdateSearch(const TQString &searchtext);
    void slotClearSearch();
    void slotRestoreMainMenu();
    void slotCaptureMainMenuGeometry();
    void paletteChanged();
    virtual void slotClear();
    virtual void slotClearOnClose();
    virtual void configChanged();
    void updateRecent();
    void slotToggleRecentMode();
    // Legacy KDE3 repair hack - no longer needed in ClassicX
    // void repairDisplay();
    void windowClearTimeout();
    void slotReinitialize();

protected:
    TQRect sideImageRect();
    TQMouseEvent translateMouseEvent(TQMouseEvent* e);
    void resizeEvent(TQResizeEvent *);
    virtual void hideEvent(TQHideEvent *);
    virtual void showEvent(TQShowEvent *);
    void paintEvent(TQPaintEvent *);
    void drawMenu(TQPainter *p, const TQRect &clipRect);
    void mousePressEvent(TQMouseEvent *);
    void mouseReleaseEvent(TQMouseEvent *);
    void mouseMoveEvent(TQMouseEvent *);
    bool loadSidePixmap();
    bool loadTopPixmap();
    void updateTopPicMask();
    void doNewSession(bool lock);
    void keyPressEvent(TQKeyEvent* e);
    virtual void focusInEvent(TQFocusEvent *e);
    bool eventFilter(TQObject *o, TQEvent *e);
    void createRecentMenuItems();
    void clearRecentSectionFromMenu();
    void refreshRecentSection();
    virtual void clearSubmenus();
    virtual void showServiceInTree(KService *service);
    void stashTreeForSearch();
    void clearSearchItems();
    void restoreStashedTree();
    void ensureSearchEdit();
    void updateSearchEditPalette();
    void showSearchBarItem();
    void buildServiceCache();
    void ensureSessionsMenu();
    void ensureLogoutMenu();
    void resetSidebarPopups();
    int userShutdownPopupHeight() const;
    void applyUserShutdownPopupSize(TQPopupMenu *menu, int targetH);

private slots:
    void slotOnShow();
    void slotShowInTreeDeferred();
    void slotRestoreStashAfterHide();
    void slotSearchItemHighlighted(int id);
    void slotSearchReturnPressed();
    void slotSidebarPopupHoverTimeout();
    void slotPopulateTreeUserMenu();
    void slotPopulateTreeShutdownMenu();
    void slotTreeUserMenuActivated(int id);

private:
    TQPopupMenu                 *sessionsMenu;
    TQPopupMenu                 *logoutMenu;
    TQPopupMenu                 *treeUserMenu;
    TQPopupMenu                 *treeShutdownMenu;
    TQPixmap                     sidePixmap;
    TQPixmap                     sideTilePixmap;
    TQPixmap                     topPixLeft;
    TQPixmap                     topPixCenter;
    TQPixmap                     topPixRight;
    int                          topPixHeight;
    int                         client_id;
    bool                        delay_init;
    TQIntDict<KickerClientMenu>  clients;
    KBookmarkMenu              *bookmarkMenu;
    TDEActionCollection          *actionCollection;
    KBookmarkOwner             *bookmarkOwner;
    PopupMenuList               dynamicSubMenus;
    TQGuardedPtr<ClassicXSearchLineEdit> searchEdit;
    TQGuardedPtr<TQObject>      m_previousKMenuObj;
    static const int            searchLineID;
    TQTimer                    *blockMouseTimer;  // Grace period timer
    TQTimer                    *popupCloseTimer;  // Grace period for sidebar popup close
    TQTimer                    *m_sidebarPopupHoverTimer; // Hover delay timer for sidebar secondary popups
    int                         m_pendingHoverBtn;
    int                         m_blockedHoverBtn;
    TQString                    m_cachedTopPicTitle;
    int                         m_lastTopPicMinute;
    int                         m_lastSidebarHeight;
    int                         m_lastSidebarWidth;
    int                         m_lastSidebarIconSize;
    int                         m_lastSidebarAlign;
    int                         m_lastSidebarUserOnTop;

    // Bools grouped together to minimize struct padding
    volatile bool               windowTimerTimedOut; // Must be volatile to prevent -O2 infinite loop
    bool                        m_inFlatSearchMode;
    bool                        m_treeStashed;
    bool                        m_servicesCached;
    bool                        m_inPopulateSessions;
    bool                        m_inPopulateLogout;
    bool                        m_inEventFilter;
    struct SearchIndexEntry {
        KService::Ptr service;
        TQString nameLower;
        TQString genericNameLower;
        TQStringList nameWords;
        TQStringList genericNameWords;
    };
    TQValueList<SearchIndexEntry> m_cachedServices;
    TQValueList<KService::Ptr>   m_searchResultsServices;
    TQValueList<int>             m_stashedTreeIds;
    TQValueList<int>             m_searchItemIds;

protected:
    struct SidebarBtn {
        int id; // 0=User, 1=Logout, 2=Settings, 3=Pictures, 4=Documents, 5=Downloads
        TQRect rect;
        TQPixmap icon;
        TQPoint iconPos;
    };
    void updateActiveSidebarButtons();
    const TQValueList<SidebarBtn>& getActiveSidebarButtons() const { return m_activeSidebarButtons; }

    int                          m_lastHighlightedId;
    TQString                     m_pendingShowInTreeMenuId;
    KService::Ptr               m_singleMatch;
    int                          m_hoveredSidebarBtn; // -1=none, 0=switchuser, 1=logout
    int m_delayedHoverBtn; // Target button index for delayed hover switch
    int                          m_currentSessionVt; // VT of currently active session
    int                          m_searchBaseHeight;
    TQPoint                     m_mainMenuPos;
    TQSize                      m_mainMenuSize;
    int                         m_savedRecentCount;
    TQValueList<SidebarBtn>     m_activeSidebarButtons;

protected slots:
    void slotClosePopupTimeout();
    void slotOpeningAnimStep();

private:
    bool     m_inOpeningAnim;
    int      m_openingAnimStep;
    int      m_totalOpeningAnimSteps;
    int      m_openingAnimDistance;
    KPanelApplet::Direction m_openingAnimDirection;
    TQTimer *m_openingAnimTimer;
    bool     m_hasTopPicMask;
    TQRegion m_baseTopPicMask;
};

#endif
