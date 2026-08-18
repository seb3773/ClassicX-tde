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

#include <time.h>

#include <tqregexp.h>
#include <tqstringlist.h>

#include <dcopclient.h>
#include <kdebug.h>
#include <ksimpleconfig.h>
#include <tdeapplication.h>

#include "classicxSettings.h"

#include "recentapps.h"

RecentlyLaunchedApps &RecentlyLaunchedApps::the() {
  static RecentlyLaunchedApps obj;
  return obj;
}

bool RecentlyLaunchedAppInfo::operator<(
    const RecentlyLaunchedAppInfo &rhs) const {
  return RecentlyLaunchedApps::the().isRuntimeRecentVsOften()
             ? m_lastLaunchTime > rhs.m_lastLaunchTime
             : m_launchCount > rhs.m_launchCount;
}

static bool isSameApp(const TQString &path1, const TQString &path2) {
  if (path1 == path2)
    return true;

  int pos1 = path1.findRev('/');
  int pos2 = path2.findRev('/');

  const TQChar *f1 = (pos1 >= 0) ? path1.unicode() + pos1 + 1 : path1.unicode();
  const TQChar *f2 = (pos2 >= 0) ? path2.unicode() + pos2 + 1 : path2.unicode();

  int len1 = (pos1 >= 0) ? path1.length() - pos1 - 1 : path1.length();
  int len2 = (pos2 >= 0) ? path2.length() - pos2 - 1 : path2.length();

  if (len1 > 0 && len1 == len2) {
    return (memcmp(f1, f2, len1 * sizeof(TQChar)) == 0);
  }
  return false;
}

RecentlyLaunchedApps::RecentlyLaunchedApps() : TQObject(0, "recently_launched_apps") {
  // set defaults
  m_nNumMenuItems = 0;
  m_bNeedToUpdate = false;
  m_bInitialised = false;
  m_bRuntimeRecentVsOften = true;
  m_bDirty = false;
  m_saveTimer = new TQTimer(this);
  connect(m_saveTimer, TQT_SIGNAL(timeout()), TQT_SLOT(slotSaveToDisk()));
}

RecentlyLaunchedApps::~RecentlyLaunchedApps() {
  if (m_bDirty) {
    slotSaveToDisk();
  }
}

void RecentlyLaunchedApps::toggleRuntimeRecentVsOften() {
  m_bRuntimeRecentVsOften = !m_bRuntimeRecentVsOften;
  qHeapSort(m_appInfos);
}

void RecentlyLaunchedApps::syncRuntimeWithSettings() {
  m_bRuntimeRecentVsOften = ClassicXSettings::recentVsOften();
  qHeapSort(m_appInfos);
}

void RecentlyLaunchedApps::init(bool forceReload) {
  if (!m_bInitialised) {
    m_bRuntimeRecentVsOften = ClassicXSettings::recentVsOften();
  }

  if (m_bInitialised && !forceReload) {
    return;
  }

  m_nNumMenuItems = 0;
  m_appInfos.clear();

  configChanged();

  KSimpleConfig kickerConfig(TQString::fromLatin1("kickerrc"));
  kickerConfig.setGroup("menus");
  TQStringList recentApps = kickerConfig.readListEntry("RecentAppsStat");

  // M4: compile once — was reconstructed on every RecentAppsStat line.
  TQRegExp re(TQString::fromLatin1("(\\d*) (\\d*) (.*)"));

  for (TQStringList::ConstIterator it = recentApps.begin();
       it != recentApps.end(); ++it) {
    if (re.search(*it) != -1) {
      int nCount = re.cap(1).toInt();
      long lTime = re.cap(2).toLong();
      TQString szPath = re.cap(3);

      bool found = false;
      for (TQValueVector<RecentlyLaunchedAppInfo>::iterator aIt =
               m_appInfos.begin();
           aIt != m_appInfos.end(); ++aIt) {
        if (isSameApp((*aIt).getDesktopPath(), szPath)) {
          found = true;
          if (time_t(lTime) > (*aIt).getLastLaunchTime()) {
            (*aIt).setLastLaunchTime(time_t(lTime));
          }
          if (nCount > (*aIt).getLaunchCount()) {
            (*aIt).setLaunchCount(nCount);
          }
          break;
        }
      }
      if (!found) {
        m_appInfos.append(
            RecentlyLaunchedAppInfo(szPath, nCount, time_t(lTime)));
      }
    }
  }

  qHeapSort(m_appInfos);
  while (m_appInfos.count() > 60) {
    m_appInfos.pop_back();
  }

  m_bInitialised = true;
}

void RecentlyLaunchedApps::configChanged() { qHeapSort(m_appInfos); }

void RecentlyLaunchedApps::save(bool forceSync) {
  m_bDirty = true;
  if (forceSync) {
    if (m_saveTimer) {
      m_saveTimer->stop();
    }
    slotSaveToDisk();
  } else {
    if (m_saveTimer && !m_saveTimer->isActive()) {
      m_saveTimer->start(2000, true); // 2000ms singleShot debounced disk write
    }
  }
}

void RecentlyLaunchedApps::slotSaveToDisk() {
  if (!m_bDirty) return;

  qHeapSort(m_appInfos);
  while (m_appInfos.count() > 60) {
    m_appInfos.pop_back();
  }

  TQStringList recentApps;

  for (unsigned int i = 0; i < m_appInfos.count(); ++i) {
    recentApps.append(TQString("%1 %2 %3")
                          .arg(m_appInfos[i].getLaunchCount())
                          .arg(m_appInfos[i].getLastLaunchTime())
                          .arg(m_appInfos[i].getDesktopPath()));
  }

  KSimpleConfig kickerConfig(TQString::fromLatin1("kickerrc"));
  kickerConfig.setGroup("menus");
  kickerConfig.writeEntry("RecentAppsStat", recentApps);
  kickerConfig.sync();

  m_bDirty = false;
}

void RecentlyLaunchedApps::appLaunched(const TQString &strApp) {
  // Inform other applications (like the quickstarter applet)
  // that an application was started
  TQByteArray params;
  TQDataStream stream(params, IO_WriteOnly);
  stream << launchDCOPSignalSource() << strApp;
  TDEApplication::kApplication()->dcopClient()->emitDCOPSignal(
      "appLauncher", "serviceStartedByStorageId(TQString,TQString)", params);

  bool found = false;
  for (TQValueVector<RecentlyLaunchedAppInfo>::iterator it = m_appInfos.begin();
       it != m_appInfos.end(); ++it) {
    if (isSameApp((*it).getDesktopPath(), strApp)) {
      (*it).increaseLaunchCount();
      (*it).setLastLaunchTime(time(0));
      found = true;
      break;
    }
  }

  if (!found) {
    m_appInfos.append(RecentlyLaunchedAppInfo(strApp, 1, time(0)));
  }

  qHeapSort(m_appInfos);
  save(false);
}

void RecentlyLaunchedApps::getRecentApps(TQStringList &recentApps) {
  recentApps.clear();

  // Re-sort according to current mode (Recent vs Most Used)
  qHeapSort(m_appInfos);

  int maximumNum = ClassicXSettings::numRecentApps();
  if (maximumNum < 2)
    maximumNum = 2;
  if (maximumNum > 6)
    maximumNum = 6;

  // Provide extra candidates in case some fail KService resolution or are
  // NoDisplay
  int candidateLimit = maximumNum + 10;
  int limit = TQMIN((int)m_appInfos.count(), candidateLimit);

  for (int i = 0; i < limit; ++i) {
    recentApps.append(m_appInfos[i].getDesktopPath());
  }
}

void RecentlyLaunchedApps::removeItem(const TQString &strName) {
  for (TQValueVector<RecentlyLaunchedAppInfo>::iterator it = m_appInfos.begin();
       it != m_appInfos.end(); ++it) {
    if ((*it).getDesktopPath() == strName) {
      m_appInfos.erase(it);
      return;
    }
  }
}

void RecentlyLaunchedApps::clearRecentApps() { m_appInfos.clear(); }

TQString RecentlyLaunchedApps::caption() const {
  return isRuntimeRecentVsOften() ? i18n("Recent applications")
                                  : i18n("Frequently used applications");
}

#include "recentapps.moc"
