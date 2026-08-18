#pragma GCC optimize ("Os")

/*****************************************************************
 * Classic-X: cached power-managerrc flags (H5).
 *****************************************************************/

#include "powermanager_flags.h"

#include <tdeconfig.h>

const PowerManagerFlags& powerManagerFlags(bool forceReload)
{
    static PowerManagerFlags flags = { false, false, true };
    static bool valid = false;
    if (!valid || forceReload) {
        TDEConfig pmcfg("power-managerrc");
        flags.disableSuspend = pmcfg.readBoolEntry("disableSuspend", false);
        flags.disableHibernate = pmcfg.readBoolEntry("disableHibernate", false);
        flags.lockOnResume = pmcfg.readBoolEntry("lockOnResume", true);
        valid = true;
    }
    return flags;
}
