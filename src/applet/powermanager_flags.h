/*****************************************************************
 * Classic-X: cached power-managerrc flags (H5).
 *****************************************************************/

#ifndef CLASSICX_POWERMANAGER_FLAGS_H
#define CLASSICX_POWERMANAGER_FLAGS_H

struct PowerManagerFlags {
    bool disableSuspend;
    bool disableHibernate;
    bool lockOnResume;
};

/** Parsed once; pass forceReload=true for settings UI. */
const PowerManagerFlags& powerManagerFlags(bool forceReload = false);

#endif // CLASSICX_POWERMANAGER_FLAGS_H
