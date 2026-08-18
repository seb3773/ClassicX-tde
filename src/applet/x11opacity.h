/*****************************************************************
 * Classic-X: _NET_WM_WINDOW_OPACITY helper (isolated Xlib use).
 *****************************************************************/

#ifndef CLASSICX_X11OPACITY_H
#define CLASSICX_X11OPACITY_H

#include <tqwindowdefs.h>

namespace ClassicX {

/**
 * Apply (or clear) window opacity via _NET_WM_WINDOW_OPACITY.
 * Caches last applied value per WId — skip redundant X round-trips.
 * opacityPercent in [0, 100): translucent; otherwise fully opaque (no property).
 */
void applyWindowOpacity(WId windowId, int opacityPercent);

/** Drop cache (e.g. after settings change that may recreate windows). */
void clearWindowOpacityCache();

} // namespace ClassicX

#endif // CLASSICX_X11OPACITY_H
