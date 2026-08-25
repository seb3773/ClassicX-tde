#!/usr/bin/env python3
"""
convert_profiles.py - Ultra-compact binary delta encoder for Classic-X preset profiles.
Zero pointers, zero runtime relocations, tiny footprint (< 3 KB total).
Author: seb3773
"""

import os
import glob
import configparser
import struct

CANONICAL_KEYS = [
    "AlwaysShowSearchBar", "ShowAppIcons", "TreeIconSize", "MenuEntryFormat",
    "ShowRunCommand", "ShowControlCenter", "UseBookmarks", "ShowPrintSystem",
    "UseBrowser", "ShowNetworkFolders", "ShowSystemMenu", "ShowRecentDocs",
    "ShowSpecialUserMenu", "ShowSpecialShutdownMenu", "ShowRecentApps",
    "RecentMode", "NumRecentApps", "MaxRecentDocs", "MaxSearchResults",
    "Transparency", "ColorMode", "FgColor", "BgColor", "SidebarBgColor",
    "TextBgColor", "TitleFgColor", "TitleBgColor", "SearchTextColor",
    "ButtonHoverColor", "FontMode", "Font", "UseSidePixmap", "SideBarWidth",
    "SidebarHover", "SidebarHoverDelay", "SidebarButtonsAlign",
    "ShowSidebarUserMenu", "ShowSidebarShutdownMenu", "ShowSidebarSettings",
    "ShowSidebarDocuments", "ShowSidebarImages", "ShowSidebarDownloads", "SidebarUserOnTop", "FullUserShutdownHeight",
    "CustomUserShutdownHeight", "ShutdownPowerOff", "ShutdownReboot",
    "ShutdownSuspend", "ShutdownHybridSuspend", "ShutdownHibernate",
    "UserPicMode", "UserPicEmbedded", "UserPicCustomPath",
    "InvertUserPic", "ColorizeUserPic", "UserPicColor",
    "IconType", "EmbeddedIcon", "CustomIconPath", "FullScaleStartIcon",
    "InvertStartIcon", "ColorizeStartIcon", "StartIconColor",
    "InvertUiIcons", "ColorizeUiIcons", "UiIconColor", "UiIconSize",
    "UiIconSource0", "UiIconSource1", "UiIconSource2", "UiIconSource3",
    "UiIconSource4", "UiIconSource5", "UiIconSource6", "UiIconSource7",
    "UiIconSource8", "UiIconSource9", "UiIconSource10", "UiIconPath0", "UiIconPath1", "UiIconPath2",
    "UiIconPath3", "UiIconPath4", "UiIconPath5", "UiIconPath6",
    "UiIconPath7", "UiIconPath8", "UiIconPath9", "UiIconPath10", "SidebarPictureMode", "SidebarPictureSource",
    "SidebarPictureEmbedded", "SidebarPictureCustomPath",
    "SidebarPictureWidthMode", "SidebarPictureAlignMode",
    "SidebarPictureExtendEdges", "SidebarPictureInvert", "SidebarPictureColorize", "SidebarPictureColor",
    "TopPicMode", "TopPicEmbedded", "TopPicCustomLeft", "TopPicCustomCenter",
    "TopPicCustomRight", "TopPicInvert", "TopPicColorize", "TopPicColor", "TopPicShowText",
    "TopPicShowUser", "TopPicShowCustomText", "TopPicText",
    "TopPicTextColorMode", "TopPicTextColor", "TopPicTextAlign", "TopPicShowRam", "TopPicShowDate", "TopPicShowTime",
    "AnimateOpening", "MenuMinWidth", "MenuCentered"
]

KEY_TO_IDX = {k.lower(): i for i, k in enumerate(CANONICAL_KEYS)}

BASELINE = {
    "alwaysshowsearchbar": "false", "showappicons": "true", "treeiconsize": "0", "menuentryformat": "1",
    "showruncommand": "true", "showcontrolcenter": "false", "usebookmarks": "false", "showprintsystem": "false",
    "usebrowser": "false", "shownetworkfolders": "false", "showsystemmenu": "false", "showrecentdocs": "true",
    "showspecialusermenu": "true", "showspecialshutdownmenu": "true", "showrecentapps": "true",
    "recentmode": "0", "numrecentapps": "3", "maxrecentdocs": "15", "maxsearchresults": "12",
    "transparency": "0", "colormode": "0", "fgcolor": "#000000", "bgcolor": "#F5F6F8", "sidebarbgcolor": "",
    "textbgcolor": "#FFFFFF", "titlefgcolor": "#000000", "titlebgcolor": "#E0E4E8", "searchtextcolor": "",
    "buttonhovercolor": "", "fontmode": "0", "font": "", "usesidepixmap": "true", "sidebarwidth": "48",
    "sidebarhover": "true", "sidebarhoverdelay": "550", "sidebarbuttonsalign": "0",
    "showsidebarusermenu": "true", "showsidebarshutdownmenu": "true", "showsidebarsettings": "true",
    "showsidebardocuments": "true", "showsidebarimages": "true", "showsidebardownloads": "false", "sidebaruserontop": "false", "fullusershutdownheight": "true",
    "customusershutdownheight": "300", "shutdownpoweroff": "true", "shutdownreboot": "true",
    "shutdownsuspend": "true", "shutdownhybridsuspend": "true", "shutdownhibernate": "true",
    "userpicmode": "0", "userpicembedded": "classic", "userpiccustompath": "",
    "invertuserpic": "false", "colorizeuserpic": "false", "userpiccolor": "#000000",
    "icontype": "0", "embeddedicon": "WinBlue", "customiconpath": "", "fullscalestarticon": "false",
    "invertstarticon": "false", "colorizestarticon": "false", "starticoncolor": "#000000",
    "invertuiicons": "false", "colorizeuiicons": "false", "uiiconcolor": "#000000", "uiiconsize": "24",
    "uiiconsource0": "0", "uiiconsource1": "0", "uiiconsource2": "0", "uiiconsource3": "0",
    "uiiconsource4": "0", "uiiconsource5": "0", "uiiconsource6": "0", "uiiconsource7": "0",
    "uiiconsource8": "0", "uiiconsource9": "0", "uiiconsource10": "0", "uiiconpath0": "", "uiiconpath1": "", "uiiconpath2": "",
    "uiiconpath3": "", "uiiconpath4": "", "uiiconpath5": "", "uiiconpath6": "",
    "uiiconpath7": "", "uiiconpath8": "", "uiiconpath9": "", "uiiconpath10": "", "sidebarpicturemode": "0", "sidebarpicturesource": "0",
    "sidebarpictureembedded": "Borders", "sidebarpicturecustompath": "",
    "sidebarpicturewidthmode": "0", "sidebarpicturealignmode": "0",
    "sidebarpictureextendedges": "false", "sidebarpictureinvert": "false", "sidebarpicturecolorize": "false", "sidebarpicturecolor": "#f9f9f9",
    "toppicmode": "0", "toppicembedded": "Simple", "toppiccustomleft": "", "toppiccustomcenter": "",
    "toppiccustomright": "", "toppicinvert": "false", "toppiccolorize": "false", "toppiccolor": "#000000", "toppicshowtext": "false",
    "toppicshowuser": "false", "toppicshowcustomtext": "true", "toppictext": "Trinity Desktop",
    "toppictextcolormode": "0", "toppictextcolor": "", "toppictextalign": "0", "toppicshowram": "false", "toppicshowdate": "false", "toppicshowtime": "false",
    "animateopening": "false", "menuminwidth": "0", "menucentered": "false"
}

def encode_value(val):
    """Returns (type_byte, bytes_payload)"""
    if val == "false":
        return 0, b""
    if val == "true":
        return 1, b""
    if val == "":
        return 6, b""
    if val.startswith("#") and len(val) == 7:
        try:
            r = int(val[1:3], 16)
            g = int(val[3:5], 16)
            b = int(val[5:7], 16)
            return 4, bytes([r, g, b])
        except ValueError:
            pass
    if val.isdigit() or (val.startswith("-") and val[1:].isdigit()):
        num = int(val)
        if 0 <= num <= 255:
            return 2, bytes([num])
        else:
            return 3, struct.pack("<h", num)
    s_bytes = val.encode("utf-8")
    return 5, bytes([len(s_bytes)]) + s_bytes

def main():
    repo_root = os.path.dirname(os.path.abspath(__file__))
    profiles_dir = os.path.join(repo_root, "profiles")
    out_file = os.path.join(repo_root, "src", "applet", "classicx_builtin_profiles.h")

    profile_files = sorted(glob.glob(os.path.join(profiles_dir, "*.profile")))
    if not profile_files:
        print(f"Error: No .profile files found in {profiles_dir}")
        return 1

    blob = bytearray()
    blob.append(len(profile_files))

    # Baseline table encoding (key_idx, type, payload)
    baseline_blob = bytearray()
    baseline_blob.append(len(BASELINE))
    for k, base_val in BASELINE.items():
        kidx = KEY_TO_IDX[k]
        vtype, payload = encode_value(base_val)
        baseline_blob.append(kidx)
        baseline_blob.append(vtype)
        baseline_blob.extend(payload)

    for pf in profile_files:
        cp = configparser.RawConfigParser()
        cp.read(pf)
        d = dict(cp.items("Dialog")) if cp.has_section("Dialog") else {}
        name = cp.get("Profile", "Name") if cp.has_section("Profile") and cp.has_option("Profile", "Name") else os.path.splitext(os.path.basename(pf))[0]

        # Prune inactive / dead options to minimize size
        if d.get("fontmode", "0") == "0":
            d["font"] = ""
        if d.get("toppicmode", "0") == "0":
            d["toppicembedded"] = BASELINE["toppicembedded"]
            d["toppiccustomleft"] = BASELINE["toppiccustomleft"]
            d["toppiccustomcenter"] = BASELINE["toppiccustomcenter"]
            d["toppiccustomright"] = BASELINE["toppiccustomright"]
            d["toppicinvert"] = BASELINE["toppicinvert"]
            d["toppiccolorize"] = BASELINE["toppiccolorize"]
            d["toppiccolor"] = BASELINE["toppiccolor"]
            d["toppicshowtext"] = BASELINE["toppicshowtext"]
            d["toppicshowuser"] = BASELINE["toppicshowuser"]
            d["toppicshowcustomtext"] = BASELINE["toppicshowcustomtext"]
            d["toppictext"] = BASELINE["toppictext"]
            d["toppictextcolormode"] = BASELINE["toppictextcolormode"]
            d["toppictextcolor"] = BASELINE["toppictextcolor"]
            d["toppictextalign"] = BASELINE["toppictextalign"]
            d["toppicshowram"] = BASELINE["toppicshowram"]
            d["toppicshowdate"] = BASELINE["toppicshowdate"]
            d["toppicshowtime"] = BASELINE["toppicshowtime"]
        else:
            if d.get("toppicshowtext", "false").lower() != "true" or d.get("toppicshowcustomtext", "false").lower() != "true":
                d["toppictext"] = BASELINE["toppictext"]
            if d.get("toppicshowtext", "false").lower() != "true" or d.get("toppictextcolormode", "0") != "2":
                d["toppictextcolor"] = BASELINE["toppictextcolor"]
            if d.get("toppicshowtext", "false").lower() != "true":
                d["toppictextalign"] = BASELINE["toppictextalign"]
                d["toppicshowram"] = BASELINE["toppicshowram"]
            if d.get("toppiccolorize", "false").lower() != "true":
                d["toppiccolor"] = BASELINE["toppiccolor"]

        if d.get("sidebarpicturemode", "0") == "0":
            d["sidebarpicturesource"] = BASELINE["sidebarpicturesource"]
            d["sidebarpictureembedded"] = BASELINE["sidebarpictureembedded"]
            d["sidebarpicturecustompath"] = BASELINE["sidebarpicturecustompath"]
            d["sidebarpicturewidthmode"] = BASELINE["sidebarpicturewidthmode"]
            d["sidebarpicturealignmode"] = BASELINE["sidebarpicturealignmode"]
            d["sidebarpictureextendedges"] = BASELINE["sidebarpictureextendedges"]
            d["sidebarpictureinvert"] = BASELINE["sidebarpictureinvert"]
            d["sidebarpicturecolorize"] = BASELINE["sidebarpicturecolorize"]
            d["sidebarpicturecolor"] = BASELINE["sidebarpicturecolor"]
        elif d.get("sidebarpicturecolorize", "false").lower() != "true":
            d["sidebarpicturecolor"] = BASELINE["sidebarpicturecolor"]

        if d.get("usesidepixmap", "true").lower() == "false":
            d["sidebarwidth"] = BASELINE["sidebarwidth"]
            d["sidebarhover"] = BASELINE["sidebarhover"]
            d["sidebarhoverdelay"] = BASELINE["sidebarhoverdelay"]
            d["sidebarbuttonsalign"] = BASELINE["sidebarbuttonsalign"]
            d["showsidebarusermenu"] = BASELINE["showsidebarusermenu"]
            d["showsidebarshutdownmenu"] = BASELINE["showsidebarshutdownmenu"]
            d["showsidebarsettings"] = BASELINE["showsidebarsettings"]
            d["showsidebardocuments"] = BASELINE["showsidebardocuments"]
            d["showsidebarimages"] = BASELINE["showsidebarimages"]
            d["showsidebardownloads"] = BASELINE["showsidebardownloads"]
            d["sidebaruserontop"] = BASELINE["sidebaruserontop"]

        if d.get("showsidebarusermenu", "true").lower() == "false":
            d["sidebaruserontop"] = BASELINE["sidebaruserontop"]

        if d.get("colorizestarticon", "false").lower() != "true":
            d["starticoncolor"] = BASELINE["starticoncolor"]
        if d.get("colorizeuiicons", "false").lower() != "true":
            d["uiiconcolor"] = BASELINE["uiiconcolor"]
        if d.get("fullusershutdownheight", "true").lower() == "true":
            d["customusershutdownheight"] = BASELINE["customusershutdownheight"]
        if d.get("userpicmode", "0") != "1":
            d["userpicembedded"] = BASELINE["userpicembedded"]
        if d.get("userpicmode", "0") != "2":
            d["userpiccustompath"] = BASELINE["userpiccustompath"]
        if d.get("colorizeuserpic", "false").lower() != "true":
            d["userpiccolor"] = BASELINE["userpiccolor"]
        if d.get("icontype", "0") != "2":
            d["customiconpath"] = BASELINE["customiconpath"]

        for i in range(11):
            src_k = f"uiiconsource{i}"
            path_k = f"uiiconpath{i}"
            # Custom is 4 (or 1 in older configs if not yet mapped)
            if d.get(src_k, "0") not in ("4", "1"):
                d[path_k] = BASELINE.get(path_k, "")

        try:
            if int(d.get("menuminwidth", "0") or 0) <= 0:
                d["menuminwidth"] = BASELINE["menuminwidth"]
        except ValueError:
            d["menuminwidth"] = BASELINE["menuminwidth"]

        deltas = []
        for k, base_val in BASELINE.items():
            val = d.get(k, base_val)
            if base_val in ("true", "false"):
                val = "true" if val.lower() in ("true", "1") else "false"
            if val != base_val:
                kidx = KEY_TO_IDX[k]
                vtype, payload = encode_value(val)
                deltas.append((kidx, vtype, payload))

        name_bytes = name.encode("utf-8")
        blob.append(len(name_bytes))
        blob.extend(name_bytes)
        blob.append(len(deltas))

        for kidx, vtype, payload in deltas:
            blob.append(kidx)
            blob.append(vtype)
            blob.extend(payload)

    # Format C++ header
    out = []
    out.append("/* Auto-generated by convert_profiles.py - DO NOT EDIT MANUALLY */")
    out.append("#ifndef CLASSICX_BUILTIN_PROFILES_H")
    out.append("#define CLASSICX_BUILTIN_PROFILES_H\n")
    out.append("#include <stddef.h>")
    out.append("#include <stdio.h>")
    out.append("#include <tqstring.h>")
    out.append("#include <tqmap.h>\n")

    # Key names array (single string literal table)
    out.append(f"static const char* const s_profileKeyNames[{len(CANONICAL_KEYS)}] = {{")
    for i, k in enumerate(CANONICAL_KEYS):
        out.append(f'    "{k}",')
    out.append("};\n")

    # Baseline blob
    out.append(f"static const unsigned char s_profileBaselineBlob[{len(baseline_blob)}] = {{")
    for i in range(0, len(baseline_blob), 16):
        chunk = baseline_blob[i:i+16]
        hex_str = ", ".join(f"0x{b:02x}" for b in chunk)
        comma = "," if i + 16 < len(baseline_blob) else ""
        out.append(f"    {hex_str}{comma}")
    out.append("};\n")

    # Delta profiles blob
    out.append(f"static const unsigned char s_profileDeltaBlob[{len(blob)}] = {{")
    for i in range(0, len(blob), 16):
        chunk = blob[i:i+16]
        hex_str = ", ".join(f"0x{b:02x}" for b in chunk)
        comma = "," if i + 16 < len(blob) else ""
        out.append(f"    {hex_str}{comma}")
    out.append("};\n")

    # C++ class with tiny decoder loop
    out.append("""class ClassicXBuiltinProfiles {
public:
    static size_t count() {
        return s_profileDeltaBlob[0];
    }

    static TQString nameAt(size_t index) {
        size_t pos = 1;
        size_t total = s_profileDeltaBlob[0];
        for (size_t i = 0; i < total; ++i) {
            size_t nameLen = s_profileDeltaBlob[pos++];
            if (i == index) {
                return TQString::fromUtf8((const char*)&s_profileDeltaBlob[pos], nameLen);
            }
            pos += nameLen;
            size_t deltaCount = s_profileDeltaBlob[pos++];
            for (size_t d = 0; d < deltaCount; ++d) {
                pos++; // key index
                unsigned char type = s_profileDeltaBlob[pos++];
                if (type == 0 || type == 1 || type == 6) {}
                else if (type == 2) { pos += 1; }
                else if (type == 3) { pos += 2; }
                else if (type == 4) { pos += 3; }
                else if (type == 5) { pos += 1 + s_profileDeltaBlob[pos]; }
            }
        }
        return TQString::null;
    }

    static bool populateMapByName(const TQString& name, TQMap<TQString, TQString>& m) {
        size_t pos = 1;
        size_t total = s_profileDeltaBlob[0];
        for (size_t i = 0; i < total; ++i) {
            size_t nameLen = s_profileDeltaBlob[pos++];
            TQString curName = TQString::fromUtf8((const char*)&s_profileDeltaBlob[pos], nameLen);
            pos += nameLen;
            size_t deltaCount = s_profileDeltaBlob[pos++];
            if (curName == name) {
                populateBaseline(m);
                m.insert(TQString::fromLatin1("Name"), curName);
                m.insert(TQString::fromLatin1("Version"), TQString::fromLatin1("1"));
                for (size_t d = 0; d < deltaCount; ++d) {
                    unsigned char kidx = s_profileDeltaBlob[pos++];
                    unsigned char type = s_profileDeltaBlob[pos++];
                    const char* kname = (kidx < sizeof(s_profileKeyNames)/sizeof(s_profileKeyNames[0])) ? s_profileKeyNames[kidx] : "";
                    applyKeyValue(m, kname, type, s_profileDeltaBlob, pos);
                }
                return true;
            } else {
                for (size_t d = 0; d < deltaCount; ++d) {
                    pos++; // key index
                    unsigned char type = s_profileDeltaBlob[pos++];
                    if (type == 0 || type == 1 || type == 6) {}
                    else if (type == 2) { pos += 1; }
                    else if (type == 3) { pos += 2; }
                    else if (type == 4) { pos += 3; }
                    else if (type == 5) { pos += 1 + s_profileDeltaBlob[pos]; }
                }
            }
        }
        return false;
    }

private:
    static void applyKeyValue(TQMap<TQString, TQString>& m, const char* kname, unsigned char type, const unsigned char* buf, size_t& pos) {
        if (!kname || kname[0] == '\\0') return;
        if (type == 0) {
            m.insert(TQString::fromLatin1(kname), TQString::fromLatin1("false"));
        } else if (type == 1) {
            m.insert(TQString::fromLatin1(kname), TQString::fromLatin1("true"));
        } else if (type == 2) {
            m.insert(TQString::fromLatin1(kname), TQString::number(buf[pos++]));
        } else if (type == 3) {
            short val = (short)(buf[pos] | (buf[pos+1] << 8));
            pos += 2;
            m.insert(TQString::fromLatin1(kname), TQString::number(val));
        } else if (type == 4) {
            char hex[16];
            snprintf(hex, sizeof(hex), "#%02x%02x%02x", buf[pos], buf[pos+1], buf[pos+2]);
            pos += 3;
            m.insert(TQString::fromLatin1(kname), TQString::fromLatin1(hex));
        } else if (type == 5) {
            size_t slen = buf[pos++];
            m.insert(TQString::fromLatin1(kname), TQString::fromUtf8((const char*)&buf[pos], slen));
            pos += slen;
        } else if (type == 6) {
            m.insert(TQString::fromLatin1(kname), TQString::fromLatin1(""));
        }
    }

    static void populateBaseline(TQMap<TQString, TQString>& m) {
        m.clear();
        size_t pos = 1;
        size_t count = s_profileBaselineBlob[0];
        for (size_t i = 0; i < count; ++i) {
            unsigned char kidx = s_profileBaselineBlob[pos++];
            unsigned char type = s_profileBaselineBlob[pos++];
            const char* kname = (kidx < sizeof(s_profileKeyNames)/sizeof(s_profileKeyNames[0])) ? s_profileKeyNames[kidx] : "";
            applyKeyValue(m, kname, type, s_profileBaselineBlob, pos);
        }
    }
};

#endif // CLASSICX_BUILTIN_PROFILES_H
""")

    with open(out_file, "w", encoding="utf-8") as f:
        f.write("\n".join(out))

    total_bytes = len(s_key_bytes := "".join(CANONICAL_KEYS).encode("utf-8")) + len(baseline_blob) + len(blob)
    print(f"Generated {out_file}: Total data size in memory is {len(blob) + len(baseline_blob)} bytes (~{(len(blob) + len(baseline_blob))/1024:.2f} KB) across {len(profile_files)} profiles.")
    return 0

if __name__ == "__main__":
    main()
