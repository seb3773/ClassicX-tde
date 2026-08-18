/*****************************************************************

Copyright (c) 2000 the kicker authors. See file AUTHORS.

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

#ifndef __recentapps_h__
#define __recentapps_h__

#include <tqvaluevector.h>
#include <tqobject.h>
#include <tqtimer.h>

class RecentlyLaunchedApps;

class RecentlyLaunchedAppInfo
{
public:
    RecentlyLaunchedAppInfo()
    {
        m_launchCount = 0;
        m_lastLaunchTime = 0;
    }

    RecentlyLaunchedAppInfo(const TQString& desktopPath, int nLaunchCount, time_t lastLaunchTime)
    {
        m_desktopPath = desktopPath;
        m_launchCount = nLaunchCount;
        m_lastLaunchTime = lastLaunchTime;
    }

    RecentlyLaunchedAppInfo(const RecentlyLaunchedAppInfo& clone)
    {
        m_desktopPath = clone.m_desktopPath;
        m_launchCount = clone.m_launchCount;
        m_lastLaunchTime = clone.m_lastLaunchTime;
    }

    bool operator<(const RecentlyLaunchedAppInfo& rhs) const;

    TQString getDesktopPath() const { return m_desktopPath; }
    int getLaunchCount() const { return m_launchCount; };
    time_t getLastLaunchTime() const { return m_lastLaunchTime; };
    void increaseLaunchCount() { m_launchCount++; };
    void setLaunchCount(int nLaunchCount) { m_launchCount = nLaunchCount; };
    void setLastLaunchTime(time_t lastLaunch) { m_lastLaunchTime = lastLaunch; };

private:
    TQString m_desktopPath;
    int m_launchCount;
    time_t m_lastLaunchTime;
};

class RecentlyLaunchedApps : public TQObject
{
    TQ_OBJECT
public:
    static RecentlyLaunchedApps& the();
    ~RecentlyLaunchedApps();
    void init(bool forceReload = false);
    void configChanged();
    void save(bool forceSync = false);
    void clearRecentApps();
    void appLaunched(const TQString & strApp);
    void getRecentApps(TQStringList & RecentApps);
    void removeItem(const TQString &strName);
    TQString caption() const;

    bool isRuntimeRecentVsOften() const { return m_bRuntimeRecentVsOften; }
    void toggleRuntimeRecentVsOften();
    void syncRuntimeWithSettings();

    int m_nNumMenuItems;
    bool m_bNeedToUpdate;

public slots:
    void slotSaveToDisk();

private:
    TQString launchDCOPSignalSource() { return "kmenu"; }
    RecentlyLaunchedApps();

    TQValueVector<RecentlyLaunchedAppInfo> m_appInfos;
    bool m_bInitialised;
    bool m_bRuntimeRecentVsOften;
    bool m_bDirty;
    TQTimer *m_saveTimer;
};

#endif
