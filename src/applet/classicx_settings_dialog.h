/*****************************************************************
 * Classic-X settings dialog (extracted from k_mnu — H13).
 *****************************************************************/

#ifndef CLASSICX_SETTINGS_DIALOG_H
#define CLASSICX_SETTINGS_DIALOG_H

#include <tqdialog.h>
#include <tqcombobox.h>
#include <tqcheckbox.h>
#include <tqspinbox.h>
#include <tqpushbutton.h>
#include <tqlabel.h>
#include <tqfont.h>
#include <tqcolor.h>
#include <tqmap.h>
#include <tqstring.h>
#include <tqstringlist.h>

class TQSlider;
class TQRadioButton;
class TDEConfig;

class ClassicXSettingsDialog : public TQDialog {
    TQ_OBJECT
public:
    ClassicXSettingsDialog(TQWidget* parent = 0);
    ~ClassicXSettingsDialog();

protected:
    virtual bool eventFilter(TQObject* watched, TQEvent* e);

private slots:
    void onShowAppIconsToggled(bool enabled);
    void onShowSidebarToggled(bool enabled);
    void onSidebarHoverToggled(bool enabled);
    void onShowRecentAppsToggled(bool enabled);
    void onShowRecentDocsToggled(bool enabled);
    void onColorModeChanged(int mode);
    void onColorButtonClicked();
    void onFontModeChanged(int mode);
    void onChooseFontClicked();
    void onOpacitySliderChanged(int val);
    void onIconTypeChanged();
    void onEmbeddedIconChanged(int index);
    void onBrowseCustomIconClicked();
    void onSidebarWidthChanged(int width);
    void onOkClicked();
    void onCancelClicked();
    void onEditMenuClicked();
    void onAboutClicked();
    void updateSpecialItemsEnableState();
    void onInvertStartIconToggled(bool enabled);
    void onColorizeStartIconToggled(bool enabled);
    void onStartIconColorClicked();
    void onInvertUiIconsToggled(bool enabled);
    void onColorizeUiIconsToggled(bool enabled);
    void onUiIconColorClicked();
    void onUiIconTypeChanged();
    void onUiIconPresetChanged(int index);
    void onBrowseUiIconClicked();
    void onSidebarPicModeChanged(int mode);
    void onSidebarPicSourceToggled(bool enabled);
    void onBrowseCustomPicClicked();
    void onSidebarPicColorizeToggled(bool enabled);
    void onSidebarPicColorClicked();
    void onTopPicModeChanged(int mode);
    void onUserShutdownHeightToggled(bool enabled);
    void onBrowseTopPicLeftClicked();
    void onBrowseTopPicCenterClicked();
    void onBrowseTopPicRightClicked();
    void onTopPicColorizeToggled(bool enabled);
    void onTopPicColorClicked();
    void onTopPicShowTextToggled(bool enabled);
    void onTopPicSubTextToggled(bool enabled);
    void onTopPicTextColorModeChanged(int index);
    void onTopPicTextColorClicked();
    void onProfileActivated(int index);
    void onProfileSaveClicked();
    void onProfileDeleteClicked();
    void onSettingsEdited();
    void updateSidebarWidthConstraints();

private:
    void updateShownAppsCountEnabled();
    void updateColorButton(TQPushButton* btn, const TQColor& col);
    void refreshColorButtons();
    void updateFontUI();
    void updateIconPreview();
    void updateUiIconPreviews();
    void syncUiIconPresetCombo();
    void updateUiIconSizeCombo(int sidebarWidth);
    bool sidebarHasButtons() const;
    void updateSidebarPictureUI();
    void updateTopPictureUI();
    void captureDialogState(TQMap<TQString, TQString>& m) const;
    void applyDialogState(const TQMap<TQString, TQString>& m);
    bool dialogStateEquals(const TQMap<TQString, TQString>& m) const;
    void writeDialogToConfig(TDEConfig& cfg) const;
    void applyConfigToDialog(TDEConfig& cfg);
    bool dialogMatchesConfig(TDEConfig& cfg) const;
    void refreshProfileList();
    void selectMatchingProfile();
    void updateProfileButtons();
    void connectProfileDirtyTracking();
    int currentUiIconSize() const;

    TQCheckBox* m_chkControlCenter;
    TQCheckBox* m_chkShowRunCommand;
    TQCheckBox* m_chkShowBookmarks;
    TQCheckBox* m_chkShowPrintSystem;
    TQCheckBox* m_chkShowQuickBrowser;
    TQCheckBox* m_chkShowNetworkFolders;
    TQCheckBox* m_chkShowSystemMenu;
    TQCheckBox* m_chkShowRecentDocs;
    TQCheckBox* m_chkShowSpecialUserMenu;
    TQCheckBox* m_chkShowSpecialShutdownMenu;
    TQCheckBox* m_chkShowRecentApps;
    TQComboBox* m_cmbRecentMode;
    TQLabel*    m_lblShownApps;
    TQSpinBox*  m_spinNumRecentApps;
    TQLabel*    m_lblMaxRecentDocs;
    TQSpinBox*  m_spinMaxRecentDocs;
    TQLabel*    m_lblMaxSearchResults;
    TQSpinBox*  m_spinMaxSearchResults;
    TQComboBox* m_cmbMenuEntryFormat;
    TQCheckBox* m_chkShowAppIcons;
    TQCheckBox* m_chkAnimateOpening;
    TQCheckBox* m_chkAlwaysShowSearchBar;
    TQLabel*    m_lblTreeIconSize;
    TQComboBox* m_cmbTreeIconSize;
    TQSlider*   m_sliderOpacity;
    TQLabel*    m_lblOpacityVal;

    TQCheckBox* m_chkShowSidebar;
    TQSpinBox*  m_spinWidth;
    TQCheckBox* m_chkSidebarHover;
    TQLabel*    m_lblSidebarHoverDelay;
    TQSpinBox*  m_spinSidebarHoverDelay;
    TQLabel*    m_lblSidebarButtonsAlign;
    TQComboBox* m_cmbSidebarButtonsAlign;

    // User & Shutdown panel height controls
    TQLabel*       m_lblUserShutdownHeight;
    TQRadioButton* m_rbUserShutdownFullHeight;
    TQRadioButton* m_rbUserShutdownCustomHeight;
    TQSpinBox*     m_spinUserShutdownCustomHeight;

    // Sidebar Picture controls
    TQLabel*       m_lblPicMode;
    TQComboBox*    m_cmbSidebarPicMode;
    TQRadioButton* m_rbPicEmbedded;
    TQComboBox*    m_cmbPicEmbedded;
    TQRadioButton* m_rbPicCustom;
    TQPushButton*  m_btnBrowseCustomPic;
    TQString       m_customPicPath;
    TQRadioButton* m_rbPicStretch;
    TQRadioButton* m_rbPicCrop;
    TQRadioButton* m_rbPicAlignTop;
    TQRadioButton* m_rbPicAlignBottom;
    TQCheckBox*    m_chkSidebarPicExtendEdges;
    TQCheckBox*    m_chkSidebarPicColorize;
    TQPushButton*  m_btnSidebarPicColor;
    TQColor        m_sidebarPicColor;

    // Top Picture controls
    TQComboBox*    m_cmbTopPicMode;
    TQComboBox*    m_cmbTopPicEmbedded;
    TQLabel*       m_lblTopPicLeftPath;
    TQPushButton*  m_btnBrowseTopPicLeft;
    TQString       m_topPicLeftPath;
    TQLabel*       m_lblTopPicCenterPath;
    TQPushButton*  m_btnBrowseTopPicCenter;
    TQString       m_topPicCenterPath;
    TQLabel*       m_lblTopPicRightPath;
    TQPushButton*  m_btnBrowseTopPicRight;
    TQString       m_topPicRightPath;
    TQCheckBox*    m_chkTopPicColorize;
    TQPushButton*  m_btnTopPicColor;
    TQColor        m_topPicColor;
    TQCheckBox*    m_chkTopPicShowText;
    TQCheckBox*    m_chkTopPicUseUser;
    TQCheckBox*    m_chkTopPicUseCustom;
    class TQLineEdit* m_editTopPicText;
    TQLabel*       m_lblTopPicTextColor;
    TQComboBox*    m_cmbTopPicTextColorMode;
    TQPushButton*  m_btnTopPicTextColor;
    TQColor        m_topPicTextColor;
    TQCheckBox*    m_chkTopPicUseDate;
    TQCheckBox*    m_chkTopPicUseTime;

    TQLabel*    m_lblButtonsHeader;
    TQCheckBox* m_chkSidebarUserMenu;
    TQCheckBox* m_chkSidebarShutdownMenu;
    TQCheckBox* m_chkSidebarSettings;
    TQCheckBox* m_chkSidebarDocuments;
    TQCheckBox* m_chkSidebarImages;

    TQCheckBox* m_chkShutdownPowerOff;
    TQCheckBox* m_chkShutdownReboot;
    TQCheckBox* m_chkShutdownSuspend;
    TQCheckBox* m_chkShutdownHybridSuspend;
    TQCheckBox* m_chkShutdownHibernate;

    TQLabel*      m_lblStartIconPreview;
    TQRadioButton* m_rbIconEmbedded;
    TQComboBox*   m_cmbEmbeddedIcon;
    TQRadioButton* m_rbIconTDE;
    TQRadioButton* m_rbIconCustom;
    TQPushButton* m_btnBrowseCustomIcon;
    TQString      m_customIconPath;
    TQCheckBox*   m_chkFullScaleStartIcon;
    TQCheckBox*   m_chkInvertStartIcon;
    TQCheckBox*   m_chkColorizeStartIcon;
    TQPushButton* m_btnStartIconColor;
    TQColor       m_startIconColor;
    TQComboBox*   m_cmbUiIconSize;
    TQCheckBox*   m_chkInvertUiIcons;
    TQCheckBox*   m_chkColorizeUiIcons;
    TQPushButton* m_btnUiIconColor;
    TQColor       m_uiIconColor;
    TQComboBox*   m_cmbUiIconPreset;

    struct UiIconControl {
        TQString label;
        TQString embeddedName;
        TQLabel* previewLabel;
        TQComboBox* cmbSource;
        TQPushButton* btnBrowse;
        TQString customPath;
    };

    static const int UI_ICON_COUNT = 9;
    UiIconControl m_uiIconControls[9];

    TQComboBox* m_cmbColorMode;

    TQPushButton* m_btnFg;
    TQPushButton* m_btnBg;
    TQPushButton* m_btnSidebarBg;
    TQPushButton* m_btnTextBg;
    TQPushButton* m_btnTitleFg;
    TQPushButton* m_btnTitleBg;
    TQPushButton* m_btnSearchText;
    TQPushButton* m_btnButtonHoverBg;

    TQColor m_fgCustom;
    TQColor m_bgCustom;
    TQColor m_sidebarBgCustom;
    TQColor m_textBgCustom;
    TQColor m_titleFgCustom;
    TQColor m_titleBgCustom;
    TQColor m_searchTextCustom;
    TQColor m_buttonHoverBgCustom;

    TQComboBox*   m_cmbFontMode;
    TQLabel*      m_lblFontPreview;
    TQPushButton* m_btnChooseFont;
    TQFont        m_customFont;

    TQComboBox*   m_cmbProfile;
    TQPushButton* m_btnProfileSave;
    TQPushButton* m_btnProfileDelete;
    TQStringList  m_profilePaths;
    bool         m_ignoreProfileDirty;
    bool         m_updatingSidebarConstraints;
};

#endif // CLASSICX_SETTINGS_DIALOG_H
