#pragma GCC optimize ("Os")

/*****************************************************************
 * Classic-X: _NET_WM_WINDOW_OPACITY helper (isolated Xlib use).
 *****************************************************************/

#include "x11opacity.h"

#include <tqmap.h>
#include <tqapplication.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>

namespace ClassicX {

static TQMap<WId, int> s_opacityCache;

void clearWindowOpacityCache()
{
    s_opacityCache.clear();
}

void applyWindowOpacity(WId windowId, int opacityPercent)
{
    if (!windowId)
        return;

    // Normalize: anything outside [0, 100) means fully opaque.
    const int want = (opacityPercent >= 0 && opacityPercent < 100) ? opacityPercent : 100;

    TQMap<WId, int>::ConstIterator it = s_opacityCache.find(windowId);
    if (it != s_opacityCache.end() && it.data() == want)
        return;

    static Atom net_wm_opacity = None;
    if (net_wm_opacity == None)
        net_wm_opacity = XInternAtom(tqt_xdisplay(), "_NET_WM_WINDOW_OPACITY", False);

    if (want < 100) {
        unsigned long opac = (unsigned long)(((unsigned long long)want * 0xffffffffUL) / 100);
        XChangeProperty(tqt_xdisplay(), windowId, net_wm_opacity, XA_CARDINAL, 32,
                        PropModeReplace, (unsigned char *)&opac, 1L);
    } else {
        // Fully opaque: only touch X if we previously set a translucent value.
        if (it != s_opacityCache.end() && it.data() < 100)
            XDeleteProperty(tqt_xdisplay(), windowId, net_wm_opacity);
        // First open at 100%: skip entirely (default is already opaque).
    }

    s_opacityCache.replace(windowId, want);
}

} // namespace ClassicX
