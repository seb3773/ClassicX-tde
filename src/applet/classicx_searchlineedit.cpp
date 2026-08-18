/*****************************************************************
 * ClassicXSearchLineEdit — Search field that keeps typed text
 * when the parent popup steals/restores focus (hover, leave).
 *****************************************************************/

#include "classicx_searchlineedit.h"

#include <tdeglobalsettings.h>
#include <tqapplication.h>
#include <tqcolor.h>
#include <tqevent.h>
#include <tqlineedit.h>
#include <tqpainter.h>
#include <tqtimer.h>

ClassicXSearchLineEdit::ClassicXSearchLineEdit(TQWidget *parent, const TQString &placeholder, const char *name)
    : KLineEdit(parent, name)
    , m_placeholderText(placeholder)
    , m_holdFocus(false)
{
    // Legacy KLineEdit / ClickLineEdit: completion + clickMessage rewrite
    // the buffer on FocusOut when the popup steals focus on hover.
    setCompletionMode(TDEGlobalSettings::CompletionNone);
    setContextMenuEnabled(false);
    setTrapReturnKey(true);
}

ClassicXSearchLineEdit::~ClassicXSearchLineEdit()
{
}

void ClassicXSearchLineEdit::setPlaceholderText(const TQString &text)
{
    m_placeholderText = text;
    update();
}

void ClassicXSearchLineEdit::setHoldFocus(bool hold)
{
    if (m_holdFocus == hold)
        return;
    m_holdFocus = hold;
    update();
}

void ClassicXSearchLineEdit::drawContents(TQPainter *p)
{
    KLineEdit::drawContents(p);

    if (text().isEmpty() && !m_placeholderText.isEmpty()) {
        TQPen oldPen = p->pen();
        p->setPen(TQColor(140, 140, 140));
        p->drawText(contentsRect(), TQt::AlignLeft | TQt::AlignVCenter, m_placeholderText);
        p->setPen(oldPen);
    }
}

void ClassicXSearchLineEdit::drawFrame(TQPainter *p)
{
    // Search view (holdFocus) always uses the selection frame. In the main
    // tree, only an explicit click on the field (hasFocus) does.
    if (!m_holdFocus && !hasFocus()) {
        KLineEdit::drawFrame(p);
        return;
    }

    TQColor sel = TDEGlobalSettings::highlightColor();
    if (!sel.isValid())
        sel = colorGroup().highlight();
    const int t = 2;
    const int w = width();
    const int h = height();
    p->fillRect(0, 0, w, t, sel);
    p->fillRect(0, h - t, w, t, sel);
    p->fillRect(0, 0, t, h, sel);
    p->fillRect(w - t, 0, t, h, sel);
}

bool ClassicXSearchLineEdit::shouldKeepFocus() const
{
    if (!m_holdFocus || !isVisible())
        return false;

    TQWidget *active = TQApplication::activePopupWidget();
    if (!active)
        return false;

    // Keep focus while our own menu is the active popup (hover items,
    // pointer leaving the field or the menu). A different popup — User /
    // Shutdown — is an allowed focus steal.
    const TQWidget *w = this;
    while (w) {
        if (w == active)
            return true;
        w = w->parentWidget();
    }
    return false;
}

void ClassicXSearchLineEdit::restoreTypedText(const TQString &kept, int cursor)
{
    if (text() == kept)
        return;
    blockSignals(true);
    setText(kept);
    if (cursor < 0)
        cursor = 0;
    if (cursor > (int)kept.length())
        cursor = (int)kept.length();
    setCursorPosition(cursor);
    blockSignals(false);
}

void ClassicXSearchLineEdit::focusInEvent(TQFocusEvent *e)
{
    const TQString kept = text();
    const int cursor = cursorPosition();
    // Skip KLineEdit: it can select-all / apply clickMessage / completion.
    TQLineEdit::focusInEvent(e);
    restoreTypedText(kept, cursor);
    if (e->reason() != TQFocusEvent::Tab && e->reason() != TQFocusEvent::Backtab)
        deselect();
    update();
}

void ClassicXSearchLineEdit::focusOutEvent(TQFocusEvent *e)
{
    const TQString kept = text();
    const int cursor = cursorPosition();
    TQLineEdit::focusOutEvent(e);
    restoreTypedText(kept, cursor);

    if (shouldKeepFocus())
        TQTimer::singleShot(0, this, TQT_SLOT(setFocus()));
    else
        update();
}

#include "classicx_searchlineedit.moc"
