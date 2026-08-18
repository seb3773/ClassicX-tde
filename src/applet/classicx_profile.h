/*****************************************************************
 * Classic-X settings profiles (built-in + user data dir, TDE-style).
 *****************************************************************/

#ifndef CLASSICX_PROFILE_H
#define CLASSICX_PROFILE_H

#include <tqstring.h>
#include <tqvaluelist.h>
#include <tqmap.h>

struct ClassicXProfileInfo {
    TQString name;
    TQString filePath;
};

class ClassicXProfile {
public:
    static TQString directory();
    static TQString sanitize(const TQString& name);
    static TQString filePathForName(const TQString& name);
    static TQValueList<ClassicXProfileInfo> list();
    static bool isBuiltin(const TQString& filePathOrUri);
    static bool isUserFile(const TQString& filePathOrUri);
    static bool removeUserFile(const TQString& filePathOrUri);
    static bool loadState(const TQString& filePathOrUri, TQMap<TQString, TQString>& outMap);
};

#endif
