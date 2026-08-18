#include "embedded_icons.h"
#include "classicx_embedded_icons.h"
#include "classicxSettings.h"
#include <ntqimage.h>
#include <kiconloader.h>
#include <tdeglobal.h>
#include <tdeconfig.h>
#include <tqfile.h>
#include <tqmap.h>

namespace EmbeddedIcons
{

static TQMap<TQString, TQPixmap> s_pixmapCache;
static TQMap<TQString, TQIconSet> s_iconSetCache;
static TQMap<TQString, TQPixmap> s_startIconCache;

void clearCache()
{
    s_pixmapCache.clear();
    s_iconSetCache.clear();
    s_startIconCache.clear();
}

void invertImage(TQImage &img)
{
    if (img.isNull()) return;

    if (img.depth() != 32) {
        img = img.convertDepth(32);
    }

    int w = img.width();
    int h = img.height();

    for (int y = 0; y < h; ++y) {
        TQRgb *line = (TQRgb*)img.scanLine(y);
        for (int x = 0; x < w; ++x) {
            TQRgb p = line[x];
            int a = tqAlpha(p);
            if (a == 0) continue;

            int r = 255 - tqRed(p);
            int g = 255 - tqGreen(p);
            int b = 255 - tqBlue(p);

            line[x] = tqRgba(r, g, b, a);
        }
    }
}

void colorizeImage(TQImage &img, const TQColor &col, bool wasInverted)
{
    if (img.isNull() || !col.isValid()) return;

    if (img.depth() != 32) {
        img = img.convertDepth(32);
    }

    int cr = col.red();
    int cg = col.green();
    int cb = col.blue();

    int w = img.width();
    int h = img.height();

    if (wasInverted) {
        // Image was inverted prior to colorizing (black -> white, white -> black).
        // Multiply inverted pixel RGB by target color (cr, cg, cb) / 255.
        // Inverted white lines (255,255,255) become (cr, cg, cb).
        // Inverted black details (0,0,0) remain black (0,0,0).
        for (int y = 0; y < h; ++y) {
            TQRgb *line = (TQRgb*)img.scanLine(y);
            for (int x = 0; x < w; ++x) {
                TQRgb p = line[x];
                int a = tqAlpha(p);
                if (a == 0) continue;

                int r = ((tqRed(p) * cr) * 257 + 257) >> 16;
                int g = ((tqGreen(p) * cg) * 257 + 257) >> 16;
                int b = ((tqBlue(p) * cb) * 257 + 257) >> 16;

                line[x] = tqRgba(r, g, b, a);
            }
        }
    } else {
        // Standard colorize for normal (non-inverted) icons.
        // Dark pixels (black lines, gray ~ 0) receive the tint color,
        // while light pixels (gray ~ 255) remain light/white.
        for (int y = 0; y < h; ++y) {
            TQRgb *line = (TQRgb*)img.scanLine(y);
            for (int x = 0; x < w; ++x) {
                TQRgb p = line[x];
                int a = tqAlpha(p);
                if (a == 0) continue;

                int gray = (tqRed(p) * 11 + tqGreen(p) * 16 + tqBlue(p) * 5) >> 5;
                int intensity = 255 - gray;

                int r = ((cr * intensity) * 257 + 257) >> 16;
                int g = ((cg * intensity) * 257 + 257) >> 16;
                int b = ((cb * intensity) * 257 + 257) >> 16;

                line[x] = tqRgba(r, g, b, a);
            }
        }
    }
}

TQStringList getStartIconNames()
{
    TQStringList list;
    for (size_t i = 0; i < classicx_embedded_icons_count; ++i) {
        if (classicx_embedded_icons_registry[i].is_start_icon()) {
            list.append(classicx_embedded_icons_registry[i].name());
        }
    }
    list.sort();
    return list;
}

TQStringList getSidebarPatternNames()
{
    TQStringList list;
    for (size_t i = 0; i < classicx_embedded_icons_count; ++i) {
        if (classicx_embedded_icons_registry[i].is_sidebar_pattern()) {
            list.append(classicx_embedded_icons_registry[i].name());
        }
    }
    list.sort();
    return list;
}

TQStringList getSidebarPictureNames()
{
    TQStringList list;
    for (size_t i = 0; i < classicx_embedded_icons_count; ++i) {
        if (classicx_embedded_icons_registry[i].is_sidebar_picture()) {
            list.append(classicx_embedded_icons_registry[i].name());
        }
    }
    list.sort();
    return list;
}

TQStringList getTopPixThemeNames()
{
    TQStringList list;
    for (size_t i = 0; i < classicx_embedded_icons_count; ++i) {
        if (classicx_embedded_icons_registry[i].is_top_pix()) {
            TQString name = classicx_embedded_icons_registry[i].name();
            if (name.endsWith("_left")) {
                name.truncate(name.length() - 5);
                if (!name.isEmpty()) {
                    name = name.left(1).upper() + name.mid(1);
                    if (!list.contains(name)) {
                        list.append(name);
                    }
                }
            }
        }
    }
    list.sort();
    return list;
}

static const ClassicXEmbeddedIconEntry* findEntry(const TQString &name)
{
    if (name.isEmpty()) return 0;
    
    TQString key = name;
    if (key.endsWith(".png") || key.endsWith(".svg") || key.endsWith(".svgz")) {
        key.truncate(key.findRev('.'));
    }
    
    for (size_t i = 0; i < classicx_embedded_icons_count; ++i) {
        if (key == classicx_embedded_icons_registry[i].name()) {
            return &classicx_embedded_icons_registry[i];
        }
    }
    return 0;
}

bool hasIcon(const TQString &name)
{
    return findEntry(name) != 0;
}

TQImage getNativeImage(const TQString &name)
{
    const ClassicXEmbeddedIconEntry* entry = findEntry(name);
    if (!entry) return TQImage();

    TQImage img;
    img.loadFromData(entry->data(), (uint)entry->size);
    return img;
}

static void uiIconSourceForName(const TQString &name, int &type, TQString &customPath)
{
    type = 0;
    customPath = TQString::null;
    if (name == "kickermenu-logout") {
        type = ClassicXSettings::shutdownIconType();
        customPath = ClassicXSettings::shutdownCustomIconPath();
    } else if (name == "menu-sleep") {
        type = ClassicXSettings::standbyIconType();
        customPath = ClassicXSettings::standbyCustomIconPath();
    } else if (name == "menu-logout") {
        type = ClassicXSettings::logoutIconType();
        customPath = ClassicXSettings::logoutCustomIconPath();
    } else if (name == "menu-restart") {
        type = ClassicXSettings::restartIconType();
        customPath = ClassicXSettings::restartCustomIconPath();
    } else if (name == "menu-hibernate") {
        type = ClassicXSettings::hibernateIconType();
        customPath = ClassicXSettings::hibernateCustomIconPath();
    } else if (name == "menu-hybrid") {
        type = ClassicXSettings::hybridSleepIconType();
        customPath = ClassicXSettings::hybridSleepCustomIconPath();
    } else if (name == "menu-docs") {
        type = ClassicXSettings::documentsIconType();
        customPath = ClassicXSettings::documentsCustomIconPath();
    } else if (name == "menu-images") {
        type = ClassicXSettings::imagesIconType();
        customPath = ClassicXSettings::imagesCustomIconPath();
    } else if (name == "menu-settings") {
        type = ClassicXSettings::settingsIconType();
        customPath = ClassicXSettings::settingsCustomIconPath();
    }
    // 0 = Win, 1 = Custom, 2 = KDE. Path is only used for Custom.
    if (type != 1)
        customPath = TQString::null;
}

TQPixmap getPixmap(const TQString &name, int width, int height, bool applyTransform)
{
    if (width <= 0 || height <= 0) {
        width = height = ClassicXSettings::uiIconSize();
        if (width < 20) width = height = 20;
    }

    bool invert = applyTransform ? ClassicXSettings::invertUiIcons() : false;
    bool colorize = applyTransform ? ClassicXSettings::colorizeUiIcons() : false;
    TQColor iconColor = ClassicXSettings::uiIconColor();

    int iconType = 0;
    TQString customPath;
    uiIconSourceForName(name, iconType, customPath);

    TQString lookupName = name;
    if (iconType == 2)
        lookupName = TQString::fromLatin1("kde_") + name;

    TQString cacheKey;
    cacheKey.reserve(80);
    cacheKey += name;
    cacheKey += '_';
    cacheKey += TQString::number(iconType);
    if (!customPath.isEmpty()) {
        cacheKey += '_';
        cacheKey += customPath;
    }
    cacheKey += '_';
    cacheKey += TQString::number(width);
    cacheKey += 'x';
    cacheKey += TQString::number(height);
    if (invert) cacheKey += "_inv";
    if (colorize && iconColor.isValid()) {
        cacheKey += "_col";
        cacheKey += iconColor.name();
    }

    TQMap<TQString, TQPixmap>::ConstIterator itPx = s_pixmapCache.find(cacheKey);
    if (itPx != s_pixmapCache.end()) {
        return itPx.data();
    }

    TQImage img;
    if (!customPath.isEmpty()) {
        if (TQFile::exists(customPath)) {
            TQPixmap px(customPath);
            if (!px.isNull()) img = px.convertToImage();
        } else {
            TQPixmap sysPx = TDEGlobal::iconLoader()->loadIcon(customPath, TDEIcon::Small, width);
            if (!sysPx.isNull()) img = sysPx.convertToImage();
        }
    }

    if (img.isNull()) {
        const ClassicXEmbeddedIconEntry* entry = findEntry(lookupName);
        if (!entry && iconType == 2)
            entry = findEntry(name);
        if (entry) {
            img.loadFromData(entry->data(), (uint)entry->size);
        } else {
            TQPixmap sysPx = TDEGlobal::iconLoader()->loadIcon(lookupName, TDEIcon::Small, width);
            if (!sysPx.isNull()) img = sysPx.convertToImage();
        }
    }

    if (width > 0 && height > 0 && (img.width() != width || img.height() != height)) {
        img = img.smoothScale(width, height);
    }

    if (invert) {
        invertImage(img);
    }

    if (colorize && iconColor.isValid()) {
        colorizeImage(img, iconColor, invert);
    }

    TQPixmap px(img);
    s_pixmapCache.insert(cacheKey, px);
    return px;
}

TQIconSet getIconSet(const TQString &name)
{
    int targetSize = ClassicXSettings::uiIconSize();
    if (targetSize < 20) targetSize = 20;

    TQString cacheKey;
    cacheKey.reserve(32);
    cacheKey += name;
    cacheKey += '_';
    cacheKey += TQString::number(targetSize);

    TQMap<TQString, TQIconSet>::ConstIterator itIco = s_iconSetCache.find(cacheKey);
    if (itIco != s_iconSetCache.end()) {
        return itIco.data();
    }

    TQPixmap px = getPixmap(name, targetSize, targetSize);
    if (px.isNull()) return TQIconSet();

    TQIconSet iconset;
    iconset.setPixmap(px, TQIconSet::Small, TQIconSet::Normal);
    iconset.setPixmap(px, TQIconSet::Small, TQIconSet::Active);
    iconset.setPixmap(px, TQIconSet::Automatic, TQIconSet::Normal);

    s_iconSetCache.insert(cacheKey, iconset);
    return iconset;
}

TQPixmap getSmallIcon(const TQString &name, int size)
{
    if (size <= 0) {
        size = ClassicXSettings::uiIconSize();
        if (size < 20) size = 20;
    }
    return getPixmap(name, size, size);
}

TQIconSet getSmallIconSet(const TQString &name)
{
    return getIconSet(name);
}

TQPixmap loadStartMenuIcon(int size)
{
    if (size <= 0) size = 32;

    int iconType = ClassicXSettings::iconType();
    TQString embeddedIcon = ClassicXSettings::embeddedIcon();
    TQString customIconPath = ClassicXSettings::customIconPath();
    bool fullScale = ClassicXSettings::fullScaleStartIcon();
    bool invert = ClassicXSettings::invertStartIcon();
    bool colorize = ClassicXSettings::colorizeStartIcon();
    TQColor iconColor = ClassicXSettings::startIconColor();

    TQString cacheKey;
    cacheKey.reserve(64);
    cacheKey += "start_";
    cacheKey += TQString::number(size);
    cacheKey += '_';
    cacheKey += TQString::number(fullScale ? 1 : 0);
    cacheKey += '_';
    cacheKey += TQString::number(iconType);
    cacheKey += '_';
    cacheKey += embeddedIcon;
    if (!customIconPath.isEmpty()) {
        cacheKey += '_';
        cacheKey += customIconPath;
    }
    if (invert) cacheKey += "_inv";
    if (colorize && iconColor.isValid()) {
        cacheKey += "_col";
        cacheKey += iconColor.name();
    }

    TQMap<TQString, TQPixmap>::ConstIterator itStart = s_startIconCache.find(cacheKey);
    if (itStart != s_startIconCache.end()) {
        return itStart.data();
    }

    TQImage img;
    if (iconType == 0) { // Embedded
        const ClassicXEmbeddedIconEntry* entry = findEntry(embeddedIcon);
        if (entry) {
            img.loadFromData(entry->data(), (uint)entry->size);
        } else {
            TQStringList startList = getStartIconNames();
            TQString fallback = startList.isEmpty() ? "TDE" : startList.first();
            const ClassicXEmbeddedIconEntry* fbEntry = findEntry(fallback);
            if (fbEntry) {
                img.loadFromData(fbEntry->data(), (uint)fbEntry->size);
            }
        }
    } else if (iconType == 1) { // TDE System Icon
        TDEConfig kickerrc("kickerrc");
        kickerrc.reparseConfiguration();
        kickerrc.setGroup("KMenu");
        TQString sysIconPath = kickerrc.readEntry("CustomIcon", "");
        if (!sysIconPath.isEmpty() && TQFile::exists(sysIconPath)) {
            TQPixmap tempPx(sysIconPath);
            if (!tempPx.isNull()) img = tempPx.convertToImage();
        }
        if (img.isNull()) {
            TQPixmap sysPx = TDEGlobal::iconLoader()->loadIcon("kmenu", TDEIcon::Panel, size);
            if (!sysPx.isNull()) img = sysPx.convertToImage();
        }
    } else if (iconType == 2) { // Custom Icon File or TDE Icon Name
        if (!customIconPath.isEmpty()) {
            if (TQFile::exists(customIconPath)) {
                TQPixmap tempPx(customIconPath);
                if (!tempPx.isNull()) img = tempPx.convertToImage();
            } else {
                TQPixmap sysPx = TDEGlobal::iconLoader()->loadIcon(customIconPath, TDEIcon::Panel, size);
                if (!sysPx.isNull()) img = sysPx.convertToImage();
            }
        }
    }

    if (img.isNull()) {
        TQStringList startList = getStartIconNames();
        TQString fallback = startList.isEmpty() ? "TDE" : startList.first();
        const ClassicXEmbeddedIconEntry* fbEntry = findEntry(fallback);
        if (fbEntry) {
            img.loadFromData(fbEntry->data(), (uint)fbEntry->size);
        }
    }

    int targetH = size;
    if (!fullScale && size >= 24) {
        TDEIconTheme *ith = TDEGlobal::iconLoader() ? TDEGlobal::iconLoader()->theme() : 0;
        int prefH = -1;
        if (ith) {
            TQValueList<int> sizes = ith->querySizes(TDEIcon::Panel);
            TQValueListConstIterator<int> it = sizes.constBegin();
            while (it != sizes.constEnd()) {
                if ((*it) + (2 * ClassicXSettings::iconMargin()) > size) {
                    break;
                }
                prefH = *it;
                ++it;
            }
        }
        if (prefH > 0 && prefH < size) {
            targetH = prefH;
        } else {
            targetH = KMAX(16, size - 8);
        }
    }

    if (targetH > 0 && !img.isNull()) {
        int w0 = img.width();
        int h0 = img.height();
        if (w0 > 0 && h0 > 0) {
            if (w0 == h0) {
                if (w0 != targetH || h0 != targetH) {
                    img = img.smoothScale(targetH, targetH);
                }
            } else {
                int newW = KMAX(1, (int)((double)w0 * ((double)targetH / (double)h0)));
                img = img.smoothScale(newW, targetH);
            }
        }
    }

    if (invert) {
        invertImage(img);
    }

    if (colorize && iconColor.isValid()) {
        colorizeImage(img, iconColor, invert);
    }

    TQPixmap px;
    px.convertFromImage(img);
    s_startIconCache.insert(cacheKey, px);
    return px;
}

} // namespace EmbeddedIcons
