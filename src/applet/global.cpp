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

#include <tqapplication.h>
#include <tqbitmap.h>
#include <tqfile.h>
#include <tqimage.h>
#include <tqpopupmenu.h>
#include <tqpainter.h>

#include <kiconloader.h>
#include <tdeglobal.h>
#include <kstandarddirs.h>
#include <kservice.h>
#include <ksimpleconfig.h>
#include <tdeglobalsettings.h>

#include <tqmap.h>

#include "global.h"
#include "classicxSettings.h"
#include "embedded_icons.h"
#include "browser_mnu.h"

namespace KickerLib
{

static TQMap<TQString, TQIconSet> s_menuIconSetCache;
static int s_padIconSize = -1;
static TQIconSet s_padIconSet;


KPanelExtension::Position directionToPosition(KPanelApplet::Direction d )
{
    switch (d)
    {
        case KPanelApplet::Down:
            return KPanelExtension::Top;
        break;

        case KPanelApplet::Left:
            return KPanelExtension::Right;
        break;

        case KPanelApplet::Right:
            return KPanelExtension::Left;
        break;

        case KPanelApplet::Up:
        default:
            return KPanelExtension::Bottom;
        break;
    }
}

KPanelExtension::Position directionToPopupPosition(KPanelApplet::Direction d)
{
    switch (d)
    {
        case KPanelApplet::Up:
            return KPanelExtension::Top;
        break;

        case KPanelApplet::Down:
            return KPanelExtension::Bottom;
        break;

        case KPanelApplet::Left:
            return KPanelExtension::Left;
        break;

        case KPanelApplet::Right:
        default:
            return KPanelExtension::Right;
        break;
    }
}

KPanelApplet::Direction positionToDirection(KPanelExtension::Position p)
{
    switch (p)
    {
        case KPanelExtension::Top:
            return KPanelApplet::Down;
        break;

        case KPanelExtension::Right:
            return KPanelApplet::Left;
        break;

        case KPanelExtension::Left:
            return KPanelApplet::Right;
        break;

        case KPanelExtension::Bottom:
        default:
            return KPanelApplet::Up;
        break;
    }
}

KPanelApplet::Direction arrowToDirection(TQt::ArrowType p)
{
    switch (p)
    {
        case Qt::DownArrow:
            return KPanelApplet::Down;
        break;

        case Qt::LeftArrow:
            return KPanelApplet::Left;
        break;

        case Qt::RightArrow:
            return KPanelApplet::Right;
        break;

        case Qt::UpArrow:
        default:
            return KPanelApplet::Up;
        break;
    }
}

int sizeValue(KPanelExtension::Size s)
{
    switch (s)
    {
        case KPanelExtension::SizeTiny:
            return 24;
        break;

        case KPanelExtension::SizeSmall:
            return 30;
        break;

        case KPanelExtension::SizeNormal:
            return 46;
        break;

        case KPanelExtension::SizeLarge:
        default:
            return 58;
        break;
    }
}

int maxButtonDim()
{
    // H9: cache kickerrc panelIconWidth (rarely changes); iconMargin may change
    // via ClassicX settings so it stays outside the cache.
    static int s_panelIconWidth = -1;
    if (s_panelIconWidth < 0) {
        KSimpleConfig kickerconfig(TQString::fromLatin1("kickerrc"));
        kickerconfig.setGroup("General");
        s_panelIconWidth = kickerconfig.readNumEntry("panelIconWidth", TDEIcon::SizeLarge);
    }
    return (2 * ClassicXSettings::iconMargin()) + s_panelIconWidth;
}

TQPopupMenu* reduceMenu(TQPopupMenu *menu)
{
    if (menu->count() != 1)
    {
       return menu;
    }

    TQMenuItem *item = menu->findItem(menu->idAt(0));

    if (item->popup())
    {
       return reduceMenu(item->popup());
    }

    return menu;
}

TQPoint popupPosition(KPanelApplet::Direction d,
                     const TQWidget* popup,
                     const TQWidget* source,
                     const TQPoint& offset)
{
    TQRect r;
    if (source->isTopLevel())
    {
        r = source->geometry();
    }
    else
    {
        r = TQRect(source->mapToGlobal(TQPoint(0, 0)),
                  source->mapToGlobal(TQPoint(source->width(), source->height())));

        switch (d)
        {
            case KPanelApplet::Left:
            case KPanelApplet::Right:
                r.setLeft( source->topLevelWidget()->x() );
                r.setWidth( source->topLevelWidget()->width() );
                break;
            case KPanelApplet::Up:
            case KPanelApplet::Down:
                r.setTop( source->topLevelWidget()->y() );
                r.setHeight( source->topLevelWidget()->height() );
                break;
        }
    }

    switch (d)
    {
        case KPanelApplet::Left:
        case KPanelApplet::Right:
        {
            TQDesktopWidget* desktop = TQApplication::desktop();
            TQRect screen = desktop->screenGeometry(desktop->screenNumber(const_cast<TQWidget*>(source)));
            int x = (d == KPanelApplet::Left) ? r.left() - popup->width() :
                                                r.right() + 1;
            int y = r.top() + offset.y();

            // try to keep this on screen
            if (y + popup->height() > screen.bottom())
            {
                y = r.bottom() - popup->height() + offset.y();

                if (y < screen.top())
                {
                    y = screen.bottom() - popup->height();

                    if (y < screen.top())
                    {
                        y = screen.top();
                    }
                }
            }

            return TQPoint(x, y);
        }
        case KPanelApplet::Up:
        case KPanelApplet::Down:
        default:
        {
            int x = 0;
            int y = (d == KPanelApplet::Up) ? r.top() - popup->height() :
                                              r.bottom() + 1;

            if (TQApplication::reverseLayout())
            {
                x = r.right() - popup->width() + 1;

                if (offset.x() > 0)
                {
                    x -= r.width() - offset.x();
                }

                // try to keep this on the screen
                if (x - popup->width() < 0)
                {
                    x = r.left();
                }

                return TQPoint(x, y);
            }
            else
            {
                TQDesktopWidget* desktop = TQApplication::desktop();
                TQRect screen = desktop->screenGeometry(desktop->screenNumber(const_cast<TQWidget*>(source)));
                x = r.left() + offset.x();

                // try to keep this on the screen
                if (x + popup->width() > screen.right())
                {
                    x = r.right() - popup->width() + 1 + offset.x();

                    if (x < screen.left())
                    {
                        x = screen.left();
                    }
                }
            }

            return TQPoint(x, y);
        }
    }
    return TQPoint(0,0);
}

TQColor shadowColor(const TQColor& c)
{
    int r = c.red();
    int g = c.green();
    int b = c.blue();

    if ( r < 128 )
        r = 255;
    else
        r = 0;

    if ( g < 128 )
        g = 255;
    else
        g = 0;

    if ( b < 128 )
        b = 255;
    else
        b = 0;

    return TQColor( r, g, b );
}

void clearMenuIconSetCache()
{
    s_menuIconSetCache.clear();
    s_padIconSize = -1;
    s_padIconSet = TQIconSet();
    PanelBrowserMenu::clearIconMap();
}

int treeIconPixelSize()
{
    // MenuEntryHeight: 0 = classic 20px, -1 = TDE Small, else exact pixels.
    // uiIconSize is sidebar chrome only — never used here.
    int h = ClassicXSettings::menuEntryHeight();
    if (h < 0) {
        h = TDEGlobal::iconLoader()->currentSize(TDEIcon::Small);
        if (h < 16)
            h = 16;
        if (h > 48)
            h = 48;
        return h;
    }
    if (h == 0)
        return 20;
    if (h < 16)
        h = 16;
    if (h > 48)
        h = 48;
    return h;
}

static void scalePixmapToSize(TQPixmap& pix, int size)
{
    if (pix.isNull() || size <= 0)
        return;
    if (pix.width() == size && pix.height() == size)
        return;
    pix.convertFromImage(TQImage(pix.convertToImage()).smoothScale(size, size));
}

// Qt3 TQIconSet(pixmap) treats max(w,h) > 22 as Large. The menu style then
// asks for Small and downscales — pads at 24/32 become shorter than hits.
// Always publish S×S on Small+Large and Normal/Active/Disabled.
static void applyTreeIconPixmaps(TQIconSet& iconset,
                                 const TQPixmap& normal,
                                 const TQPixmap& active)
{
    const TQPixmap& act = active.isNull() ? normal : active;
    iconset.setPixmap(normal, TQIconSet::Small, TQIconSet::Normal);
    iconset.setPixmap(act, TQIconSet::Small, TQIconSet::Active);
    iconset.setPixmap(normal, TQIconSet::Small, TQIconSet::Disabled);
    iconset.setPixmap(normal, TQIconSet::Large, TQIconSet::Normal);
    iconset.setPixmap(act, TQIconSet::Large, TQIconSet::Active);
    iconset.setPixmap(normal, TQIconSet::Large, TQIconSet::Disabled);
}

TQIconSet treeIconPadIconSet()
{
    const int S = treeIconPixelSize();
    if (S == s_padIconSize && !s_padIconSet.isNull())
        return s_padIconSet;

    TQPixmap pix(S, S);
    pix.fill(TQColor(0, 0, 0));
    TQBitmap mask(S, S);
    mask.fill(TQt::color0);
    pix.setMask(mask);

    TQIconSet iconset;
    applyTreeIconPixmaps(iconset, pix, pix);
    s_padIconSize = S;
    s_padIconSet = iconset;
    return iconset;
}

TQIconSet menuIconSet(const TQString& icon)
{
    if (EmbeddedIcons::hasIcon(icon)) {
        // Embedded UI icons (sidebar / chrome) follow uiIconSize.
        return EmbeddedIcons::getIconSet(icon);
    }

    if (!ClassicXSettings::showAppIcons()) {
        return TQIconSet();
    }

    const int iconSize = treeIconPixelSize();

    TQString cacheKey;
    cacheKey.reserve(32);
    cacheKey += icon;
    cacheKey += '_';
    cacheKey += TQString::number(iconSize);

    TQMap<TQString, TQIconSet>::ConstIterator it = s_menuIconSetCache.find(cacheKey);
    if (it != s_menuIconSetCache.end()) {
        return it.data();
    }

    TQIconSet iconset;

    if (icon != "unknown")
    {
        TQPixmap normal = TDEGlobal::iconLoader()->loadIcon(icon,
                                                     TDEIcon::NoGroup,
                                                     iconSize,
                                                     TDEIcon::DefaultState,
                                                     0,
                                                     true);

        TQPixmap active = TDEGlobal::iconLoader()->loadIcon(icon,
                                                     TDEIcon::NoGroup,
                                                     iconSize,
                                                     TDEIcon::ActiveState,
                                                     0,
                                                     true);

        // Hits and search pads share this size: always force S×S (up and down).
        scalePixmapToSize(normal, iconSize);
        scalePixmapToSize(active, iconSize);

        if (!normal.isNull())
            applyTreeIconPixmaps(iconset, normal, active);
    }

    if (iconset.isNull())
        iconset = treeIconPadIconSet();

    s_menuIconSetCache.insert(cacheKey, iconset);
    return iconset;
}

static bool s_colorCacheValid = false;
static TQColor s_cachedFgColor;
static TQColor s_cachedBgColor;
static TQColor s_cachedSidebarBgColor;
static TQColor s_cachedTextBgColor;
static TQColor s_cachedSearchTextColor;
static TQColor s_cachedTitleFgColor;
static TQColor s_cachedTitleBgColor;
static TQColor s_cachedButtonHoverColor;
static int s_cachedFontMode = 0;
static TQFont s_cachedFont;
static TQPixmap s_cachedSidebarTile;
static TQString s_cachedSidebarTileKey;

void clearColorCache()
{
    s_colorCacheValid = false;
    s_cachedSidebarTile = TQPixmap();
    s_cachedSidebarTileKey = TQString::null;
}

static void ensureColorCache()
{
    if (s_colorCacheValid) return;

    TDEConfig config("classicxapplet_rc");
    config.setGroup("Colors");
    int mode = config.readNumEntry("ColorMode", 0);

    if (mode == 0) { // Default
        s_cachedFgColor = TQColor(0, 0, 0);
        s_cachedBgColor = TQColor(245, 246, 248);
        s_cachedSidebarBgColor = TQColor(245, 246, 248);
        s_cachedTextBgColor = TQColor(255, 255, 255);
        s_cachedSearchTextColor = TQColor(0, 0, 0);
        s_cachedTitleFgColor = TQColor(0, 0, 0);
        s_cachedTitleBgColor = TQColor(220, 224, 230);
        s_cachedButtonHoverColor = TQColor(225, 230, 238);
    } else if (mode == 1) { // TDE System
        s_cachedFgColor = TQApplication::palette().active().text();
        s_cachedBgColor = TQApplication::palette().active().background();
        s_cachedSidebarBgColor = TQApplication::palette().active().background();
        s_cachedTextBgColor = TQApplication::palette().active().base();
        s_cachedSearchTextColor = TQApplication::palette().active().text();
        s_cachedTitleFgColor = TQApplication::palette().active().buttonText();
        s_cachedTitleBgColor = TQApplication::palette().active().button();
        s_cachedButtonHoverColor = TQApplication::palette().active().highlight();
    } else { // Custom
        s_cachedFgColor = TQColor(config.readEntry("FgColor", "#000000"));
        s_cachedBgColor = TQColor(config.readEntry("BgColor", "#F5F6F8"));
        TQString sideBgStr = config.readEntry("SidebarBgColor", "");
        s_cachedSidebarBgColor = !sideBgStr.isEmpty() ? TQColor(sideBgStr) : s_cachedBgColor;
        s_cachedTextBgColor = TQColor(config.readEntry("TextBgColor", "#FFFFFF"));
        TQString searchTextStr = config.readEntry("SearchTextColor", "");
        if (!searchTextStr.isEmpty())
            s_cachedSearchTextColor = TQColor(searchTextStr);
        else
            s_cachedSearchTextColor = s_cachedFgColor;
        s_cachedTitleFgColor = TQColor(config.readEntry("TitleFgColor", "#000000"));
        s_cachedTitleBgColor = TQColor(config.readEntry("TitleBgColor", "#E0E4E8"));
        TQString btnHoverStr = config.readEntry("ButtonHoverColor", "");
        if (!btnHoverStr.isEmpty()) {
            s_cachedButtonHoverColor = TQColor(btnHoverStr);
        } else {
            s_cachedButtonHoverColor = TQColor(225, 230, 238);
        }
    }

    config.setGroup("Font");
    s_cachedFontMode = config.readNumEntry("FontMode", 0);
    TQFont sysFont = TDEGlobalSettings::menuFont();
    if (s_cachedFontMode == 1) {
        TQString fontStr = config.readEntry("Font", sysFont.toString());
        if (fontStr.isEmpty() || !s_cachedFont.fromString(fontStr)) {
            s_cachedFont = sysFont;
        }
    } else {
        s_cachedFont = sysFont;
    }

    s_colorCacheValid = true;
}

TQColor getMenuFgColor()
{
    ensureColorCache();
    return s_cachedFgColor;
}

TQColor getClassicKMenuBgColor()
{
    ensureColorCache();
    return s_cachedBgColor;
}

TQColor getClassicKMenuSidebarBgColor()
{
    ensureColorCache();
    return s_cachedSidebarBgColor;
}

TQColor getMenuTextBgColor()
{
    ensureColorCache();
    return s_cachedTextBgColor;
}

TQColor getMenuSearchTextColor()
{
    ensureColorCache();
    return s_cachedSearchTextColor;
}

TQColor getMenuTitleFgColor()
{
    ensureColorCache();
    return s_cachedTitleFgColor;
}

TQColor getMenuTitleBgColor()
{
    ensureColorCache();
    return s_cachedTitleBgColor;
}

TQColor getKMenuButtonHoverColor()
{
    ensureColorCache();
    return s_cachedButtonHoverColor;
}

static void colorizeSidebarWorkingImage(TQImage &img, bool on, const TQColor &col)
{
    if (on && col.isValid())
        EmbeddedIcons::colorizeImage(img, col, true);
}

TQPixmap getSidebarTilePixmap(int sidebarWidth, int menuHeight)
{
    if (sidebarWidth <= 0) sidebarWidth = 48;
    if (menuHeight <= 0) menuHeight = 500;

    ensureColorCache();

    TDEConfig config("classicxapplet_rc");
    config.setGroup("Sidebar");
    int mode = config.readNumEntry("SidebarPictureMode", 0); // 0 = None, 1 = Pattern, 2 = Picture
    if (mode == 0) {
        return TQPixmap();
    }

    int source = config.readNumEntry("SidebarPictureSource", 0); // 0 = Embedded, 1 = Custom
    TQString embName = config.readEntry("SidebarPictureEmbedded", "Chevron");
    TQString customPath = config.readEntry("SidebarPictureCustomPath", "");
    int widthMode = config.readNumEntry("SidebarPictureWidthMode", 0); // 0 = Stretch, 1 = Crop
    int alignMode = config.readNumEntry("SidebarPictureAlignMode", 0); // 0 = AlignTop, 1 = AlignBottom
    bool extendEdges = config.readBoolEntry("SidebarPictureExtendEdges", false);
    bool colorize = config.readBoolEntry("SidebarPictureColorize", false);
    TQColor picColor = TQColor(config.readEntry("SidebarPictureColor",
        TDEGlobalSettings::highlightColor().name()));
    if (!picColor.isValid())
        picColor = TDEGlobalSettings::highlightColor();
    TQColor sidebarBg = getClassicKMenuSidebarBgColor();

    TQString key = TQString("%1_%2_%3_%4_%5_%6_%7_%8_%9_%10")
                   .arg(sidebarWidth).arg(menuHeight).arg(mode).arg(source)
                   .arg(embName).arg(customPath).arg(widthMode).arg(alignMode)
                   .arg(sidebarBg.name()).arg(extendEdges ? 1 : 0);
    if (colorize && picColor.isValid()) {
        key += "_col";
        key += picColor.name();
    }

    if (!s_cachedSidebarTile.isNull() && s_cachedSidebarTileKey == key) {
        return s_cachedSidebarTile;
    }

    TQImage srcImg;
    if (source == 0) {
        srcImg = EmbeddedIcons::getNativeImage(embName);
    } else if (!customPath.isEmpty() && TQFile::exists(customPath)) {
        srcImg.load(customPath);
    }

    if (srcImg.isNull() || srcImg.width() <= 0 || srcImg.height() <= 0) {
        s_cachedSidebarTile = TQPixmap();
        s_cachedSidebarTileKey = key;
        return s_cachedSidebarTile;
    }

    int w0 = srcImg.width();
    int h0 = srcImg.height();

    if (mode == 1) { // Pattern mode
        if (widthMode == 0) { // Stretch: scale width to sidebarWidth (proportional height)
            int newH = KMAX(1, (int)((double)h0 * ((double)sidebarWidth / (double)w0)));
            TQImage tileImg = srcImg.smoothScale(sidebarWidth, newH);
            colorizeSidebarWorkingImage(tileImg, colorize, picColor);

            TQPixmap tile(sidebarWidth, newH);
            tile.fill(sidebarBg);

            TQPainter p(&tile);
            p.drawImage(0, 0, tileImg);
            p.end();

            s_cachedSidebarTile = tile;
            s_cachedSidebarTileKey = key;
            return s_cachedSidebarTile;
        } else { // Crop: keep native size, crop/center horizontally
            colorizeSidebarWorkingImage(srcImg, colorize, picColor);
            TQPixmap tile(sidebarWidth, h0);
            tile.fill(sidebarBg);

            TQPainter p(&tile);
            if (w0 > sidebarWidth) {
                int offX = (w0 - sidebarWidth) / 2;
                p.drawImage(0, 0, srcImg, offX, 0, sidebarWidth, h0);
            } else {
                int offX = (sidebarWidth - w0) / 2;
                p.drawImage(offX, 0, srcImg);
            }
            p.end();

            s_cachedSidebarTile = tile;
            s_cachedSidebarTileKey = key;
            return s_cachedSidebarTile;
        }
    } else if (mode == 2) { // Picture mode
        TQPixmap pic(sidebarWidth, menuHeight);
        pic.fill(sidebarBg);

        TQImage scaledImg = srcImg;
        if (widthMode == 0 && w0 != sidebarWidth) {
            double ratio = (double)sidebarWidth / (double)w0;
            int newH = KMAX(1, (int)(h0 * ratio));
            scaledImg = srcImg.smoothScale(sidebarWidth, newH);
        }
        colorizeSidebarWorkingImage(scaledImg, colorize, picColor);

        int drawY = 0;
        if (alignMode == 1) { // Align to bottom
            drawY = menuHeight - scaledImg.height();
        }

        TQPainter p(&pic);
        int srcX = 0;
        int imgW = scaledImg.width();
        int imgH = scaledImg.height();
        int drawX = 0;

        if (widthMode == 1 && imgW > sidebarWidth) {
            srcX = (imgW - sidebarWidth) / 2;
            imgW = sidebarWidth;
        } else {
            drawX = (sidebarWidth - imgW) / 2;
            if (drawX < 0) drawX = 0;
        }

        // Draw main image
        if (widthMode == 1 && scaledImg.width() > sidebarWidth) {
            p.drawImage(0, drawY, scaledImg, srcX, 0, sidebarWidth, imgH);
        } else {
            p.drawImage(drawX, drawY, scaledImg);
        }

        // Extend 1px edge rows to fill remaining height if enabled
        if (extendEdges) {
            if (alignMode == 1 && drawY > 0) { // Align to bottom -> extend top 1px row upwards
                TQImage topRow = scaledImg.copy(srcX, 0, imgW, 1);
                if (!topRow.isNull()) {
                    TQImage topStretched = topRow.smoothScale(imgW, drawY);
                    p.drawImage(drawX, 0, topStretched);
                }
            } else if (alignMode == 0 && (drawY + imgH) < menuHeight) { // Align to top -> extend bottom 1px row downwards
                int botY = drawY + imgH;
                int remainingH = menuHeight - botY;
                if (remainingH > 0) {
                    TQImage botRow = scaledImg.copy(srcX, imgH - 1, imgW, 1);
                    if (!botRow.isNull()) {
                        TQImage botStretched = botRow.smoothScale(imgW, remainingH);
                        p.drawImage(drawX, botY, botStretched);
                    }
                }
            }
        }

        p.end();

        s_cachedSidebarTile = pic;
        s_cachedSidebarTileKey = key;
        return s_cachedSidebarTile;
    }

    return TQPixmap();
}

void updateMenuPalette(TQPopupMenu *menu)
{
    if (!menu) return;

    ensureColorCache();
    menu->setFont(s_cachedFont);

    TQColor bg = s_cachedBgColor;
    TQColor fg = s_cachedFgColor;

    menu->setBackgroundMode(TQWidget::PaletteBackground);
    menu->setBackgroundOrigin(TQWidget::WidgetOrigin);
    menu->setPaletteBackgroundColor(bg);
    menu->setPaletteForegroundColor(fg);

    TQPalette pal = menu->palette();

    TQColorGroup cgActive = pal.active();
    cgActive.setColor(TQColorGroup::Foreground, fg);
    cgActive.setBrush(TQColorGroup::Foreground, TQBrush(fg));
    cgActive.setColor(TQColorGroup::Text, fg);
    cgActive.setBrush(TQColorGroup::Text, TQBrush(fg));
    cgActive.setColor(TQColorGroup::ButtonText, fg);
    cgActive.setBrush(TQColorGroup::ButtonText, TQBrush(fg));

    cgActive.setColor(TQColorGroup::Background, bg);
    cgActive.setBrush(TQColorGroup::Background, TQBrush(bg));
    cgActive.setColor(TQColorGroup::Button, bg);
    cgActive.setBrush(TQColorGroup::Button, TQBrush(bg));
    cgActive.setColor(TQColorGroup::Base, bg);
    cgActive.setBrush(TQColorGroup::Base, TQBrush(bg));

    pal.setActive(cgActive);
    pal.setInactive(cgActive);
    pal.setDisabled(cgActive);

    menu->setPalette(pal);
}

} // namespace

int kicker_screen_number = 0;


