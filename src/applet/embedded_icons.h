#ifndef EMBEDDED_ICONS_H
#define EMBEDDED_ICONS_H

#include <tqpixmap.h>
#include <tqiconset.h>
#include <tqstring.h>

namespace EmbeddedIcons
{
    TQStringList getStartIconNames();
    TQStringList getSidebarPatternNames();
    TQStringList getSidebarPictureNames();
    TQStringList getTopPixThemeNames();
    bool hasIcon(const TQString &name);
    TQImage getNativeImage(const TQString &name);
    TQPixmap getPixmap(const TQString &name, int width = 0, int height = 0, bool applyTransform = true);
    TQIconSet getIconSet(const TQString &name);

    TQPixmap getSmallIcon(const TQString &name, int size = 0);
    TQIconSet getSmallIconSet(const TQString &name);
    TQPixmap loadStartMenuIcon(int size = 32);
    void invertImage(TQImage &img);
    void colorizeImage(TQImage &img, const TQColor &col, bool wasInverted = false);
    void clearCache();
}

#endif // EMBEDDED_ICONS_H
