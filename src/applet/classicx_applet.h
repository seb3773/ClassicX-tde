#ifndef CLASSICX_APPLET_H
#define CLASSICX_APPLET_H

#include <kpanelapplet.h>
#include "panelbutton.h"
#include "k_mnu.h"

class ClassicXButton : public PanelPopupButton
{
    TQ_OBJECT

public:
    ClassicXButton(TQWidget *parent);
    ~ClassicXButton();

    void reloadIcon() { loadIcons(); update(); }
    void setInitialized(bool init) { PanelPopupButton::setInitialized(init); }
    void configure();

    virtual void showMenu();
    void realShowMenu() { PanelPopupButton::showMenu(); }

protected:
    virtual TQString tileName() { return "KMenu"; }
    virtual TQString defaultIcon() const { return "kmenu"; }
    virtual void mousePressEvent(TQMouseEvent *e);
    virtual void mouseReleaseEvent(TQMouseEvent *e);

protected slots:
    void slotConfigure();
    void slotEditMenu();
    void slotConfigurePanel();
};

class ClassicXApplet : public KPanelApplet
{
    TQ_OBJECT

public:
    ClassicXApplet(const TQString& configFile, Type type = Normal, int actions = 0,
                   TQWidget *parent = 0, const char *name = 0);
    ~ClassicXApplet();

    int widthForHeight(int height) const;
    int heightForWidth(int width) const;

    int panelHeight() const {
        if (m_panelHeight > 0) return m_panelHeight;
        if (height() > 0) return height();
        if (parentWidget() && parentWidget()->height() > 0) return parentWidget()->height();
        return 42;
    }
    void notifyLayoutChanged() { emit updateLayout(); }

protected:
    virtual void positionChange(Position p);

public slots:
    void showMenu();
    void updateIcon();
    void preferences();

private:
    mutable int m_panelHeight;
    ClassicXButton *m_button;
    PanelKMenu *m_menu;
    /** MenuManager stub target before we hijacked it; not owned by us. */
    PanelKMenu *m_menuManagerPrevious;
};

#endif // CLASSICX_APPLET_H
