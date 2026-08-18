/*****************************************************************
 * Classic-X settings profiles (built-in + user data dir, TDE-style).
 *****************************************************************/

#pragma GCC optimize ("Os")

#include "classicx_profile.h"
#include "classicx_builtin_profiles.h"

#include <tqdir.h>
#include <tqfile.h>
#include <tqfileinfo.h>
#include <tqmap.h>

#include <ksimpleconfig.h>
#include <tdeglobal.h>
#include <kstandarddirs.h>

TQString ClassicXProfile::directory()
{
    return TDEGlobal::dirs()->saveLocation("data", "classicxapplet/profiles/");
}

TQString ClassicXProfile::sanitize(const TQString& name)
{
    TQString s;
    const TQString trimmed = name.stripWhiteSpace();
    for (unsigned int i = 0; i < trimmed.length(); ++i) {
        const TQChar c = trimmed[i];
        if (c.isLetterOrNumber() || c == TQChar('-') || c == TQChar('_') || c == TQChar('.'))
            s += c;
        else if (c.isSpace())
            s += TQChar('_');
    }
    while (s.startsWith("."))
        s = s.mid(1);
    if (s.isEmpty())
        s = TQString::fromLatin1("profile");
    return s;
}

TQString ClassicXProfile::filePathForName(const TQString& name)
{
    return directory() + sanitize(name) + TQString::fromLatin1(".profile");
}

static TQString profileDisplayName(const TQString& filePath)
{
    KSimpleConfig cfg(filePath, true);
    cfg.setGroup("Profile");
    TQString name = cfg.readEntry("Name");
    if (name.isEmpty()) {
        TQFileInfo fi(filePath);
        name = fi.baseName();
    }
    return name;
}

static void addProfilesFromDir(const TQString& dirPath,
                               TQMap<TQString, ClassicXProfileInfo>& byKey,
                               bool overwrite)
{
    TQDir dir(dirPath);
    if (!dir.exists())
        return;

    const TQStringList files = dir.entryList(TQString::fromLatin1("*.profile"), TQDir::Files);
    for (TQStringList::ConstIterator it = files.begin(); it != files.end(); ++it) {
        ClassicXProfileInfo info;
        info.filePath = dir.absFilePath(*it);
        info.name = profileDisplayName(info.filePath);
        const TQString key = info.name.lower();
        if (!overwrite && byKey.contains(key))
            continue;
        byKey[key] = info;
    }
}

TQValueList<ClassicXProfileInfo> ClassicXProfile::list()
{
    TQMap<TQString, ClassicXProfileInfo> byKey;

    // 1. Add all built-in profiles
    const size_t totalBuiltin = ClassicXBuiltinProfiles::count();
    for (size_t i = 0; i < totalBuiltin; ++i) {
        TQString name = ClassicXBuiltinProfiles::nameAt(i);
        if (name.isEmpty())
            continue;
        ClassicXProfileInfo info;
        info.name = name;
        info.filePath = TQString::fromLatin1("builtin:") + info.name;
        byKey[info.name.lower()] = info;
    }

    // 2. Add system / user data directory profiles from disk
    const TQString userDir = directory();
    const TQStringList dirs = TDEGlobal::dirs()->findDirs("data", "classicxapplet/profiles/");
    for (TQStringList::ConstIterator it = dirs.begin(); it != dirs.end(); ++it) {
        TQString d = *it;
        if (!d.endsWith("/"))
            d += "/";
        if (d == userDir)
            continue;
        addProfilesFromDir(d, byKey, false);
    }
    addProfilesFromDir(userDir, byKey, true);

    // 3. Sort alphabetically by name
    TQValueList<ClassicXProfileInfo> result;
    TQMap<TQString, ClassicXProfileInfo>::ConstIterator it = byKey.begin();
    for (; it != byKey.end(); ++it) {
        const ClassicXProfileInfo& info = it.data();
        TQValueList<ClassicXProfileInfo>::Iterator ins = result.begin();
        while (ins != result.end() && (*ins).name.lower() < info.name.lower())
            ++ins;
        result.insert(ins, info);
    }
    return result;
}

bool ClassicXProfile::isBuiltin(const TQString& filePathOrUri)
{
    return filePathOrUri.startsWith(TQString::fromLatin1("builtin:"));
}

bool ClassicXProfile::isUserFile(const TQString& filePathOrUri)
{
    if (filePathOrUri.isEmpty() || isBuiltin(filePathOrUri))
        return false;
    return filePathOrUri.startsWith(directory());
}

bool ClassicXProfile::removeUserFile(const TQString& filePathOrUri)
{
    if (!isUserFile(filePathOrUri))
        return false;
    return TQFile::remove(filePathOrUri);
}

bool ClassicXProfile::loadState(const TQString& filePathOrUri, TQMap<TQString, TQString>& outMap)
{
    outMap.clear();
    if (filePathOrUri.isEmpty())
        return false;

    if (isBuiltin(filePathOrUri)) {
        const TQString name = filePathOrUri.mid(8); // length of "builtin:"
        return ClassicXBuiltinProfiles::populateMapByName(name, outMap);
    }

    if (TQFile::exists(filePathOrUri)) {
        KSimpleConfig cfg(filePathOrUri, true);
        outMap = cfg.entryMap("Dialog");
        return !outMap.isEmpty();
    }

    return false;
}
