/*****************************************************************

Copyright (c) 2006 Dirk Mueller <mueller@kde.org>

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

#include "k_mnu_stub.h"
#include "k_mnu.h"

void KMenuStub::setPanelKMenu(PanelKMenu* menu)
{
    m_type = t_PanelKMenu;
    m_w.panelkmenu = menu;
}

PanelKMenu* KMenuStub::panelKMenu() const
{
    if (m_type != t_PanelKMenu)
        return 0;
    return m_w.panelkmenu;
}

void KMenuStub::removeClientMenu(int id)
{
    if (m_type == t_PanelKMenu && m_w.panelkmenu)
        m_w.panelkmenu->removeClientMenu(id);
}

int KMenuStub::insertClientMenu(KickerClientMenu *p)
{
    if (m_type != t_PanelKMenu || !m_w.panelkmenu)
        return -1;
    return m_w.panelkmenu->insertClientMenu(p);
}

void KMenuStub::adjustSize()
{
    if (m_type == t_PanelKMenu && m_w.panelkmenu)
        m_w.panelkmenu->adjustSize();
}

void KMenuStub::hide()
{
    if (m_type == t_PanelKMenu && m_w.panelkmenu)
        m_w.panelkmenu->hide();
}

void KMenuStub::show()
{
    if (m_type == t_PanelKMenu && m_w.panelkmenu)
        m_w.panelkmenu->show();
}

void KMenuStub::showMenu()
{
    if (m_type == t_PanelKMenu && m_w.panelkmenu)
        m_w.panelkmenu->showMenu();
}

void KMenuStub::popup(const TQPoint &pos, int indexAtPoint)
{
    if (m_type == t_PanelKMenu && m_w.panelkmenu)
        m_w.panelkmenu->popup(pos, indexAtPoint);
}

void KMenuStub::selectFirstItem()
{
    if (m_type == t_PanelKMenu && m_w.panelkmenu)
        m_w.panelkmenu->selectFirstItem();
}

void KMenuStub::resize(int w, int h)
{
    if (m_type == t_PanelKMenu && m_w.panelkmenu)
        m_w.panelkmenu->resize(w, h);
}

TQSize KMenuStub::sizeHint() const
{
    if (m_type != t_PanelKMenu || !m_w.panelkmenu)
        return TQSize();
    return m_w.panelkmenu->sizeHint();
}

bool KMenuStub::highlightMenuItem(const TQString &menuId)
{
    if (m_type != t_PanelKMenu || !m_w.panelkmenu)
        return false;
    return m_w.panelkmenu->highlightMenuItem(menuId);
}

void KMenuStub::clearRecentMenuItems()
{
    if (m_type == t_PanelKMenu && m_w.panelkmenu)
        m_w.panelkmenu->clearRecentMenuItems();
}

void KMenuStub::initialize()
{
    if (m_type == t_PanelKMenu && m_w.panelkmenu)
        m_w.panelkmenu->initialize();
}

bool KMenuStub::isVisible() const
{
    if (m_type != t_PanelKMenu || !m_w.panelkmenu)
        return false;
    return m_w.panelkmenu->isVisible();
}

TQWidget* KMenuStub::widget()
{
    if (m_type != t_PanelKMenu)
        return 0;
    return m_w.panelkmenu;
}
