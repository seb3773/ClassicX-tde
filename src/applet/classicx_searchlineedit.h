/*****************************************************************
 * ClassicXSearchLineEdit — Search field that keeps typed text
 * when the parent popup steals/restores focus (hover, leave).
 *****************************************************************/

#ifndef CLASSICX_SEARCHLINEEDIT_H
#define CLASSICX_SEARCHLINEEDIT_H

#include <klineedit.h>
#include <tqstring.h>

class TQPainter;
class TQFocusEvent;

class ClassicXSearchLineEdit : public KLineEdit
{
    TQ_OBJECT
public:
    ClassicXSearchLineEdit(TQWidget *parent = 0, const TQString &placeholder = TQString::null, const char *name = 0);
    virtual ~ClassicXSearchLineEdit();

    void setPlaceholderText(const TQString &text);
    TQString placeholderText() const { return m_placeholderText; }

    // Search view: keep the caret and paint the selection frame even if
    // TQt3 briefly steals focus (hover). Off in the main tree view.
    void setHoldFocus(bool hold);
    bool holdFocus() const { return m_holdFocus; }

protected:
    virtual void drawContents(TQPainter *p);
    virtual void drawFrame(TQPainter *p);
    virtual void focusInEvent(TQFocusEvent *e);
    virtual void focusOutEvent(TQFocusEvent *e);

private:
    bool shouldKeepFocus() const;
    void restoreTypedText(const TQString &kept, int cursor);

    TQString m_placeholderText;
    bool m_holdFocus;
};

#endif // CLASSICX_SEARCHLINEEDIT_H
