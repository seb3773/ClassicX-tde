#!/bin/bash
set -e

PACKAGE_NAME="tde-kicker-classicx-applet"
INSTALL_FLAG=0
VERSION_ARG=""

for arg in "$@"; do
    if [ "$arg" = "--install" ] || [ "$arg" = "-i" ]; then
        INSTALL_FLAG=1
    elif [ -z "$VERSION_ARG" ] && [ "${arg#-}" = "$arg" ]; then
        VERSION_ARG="$arg"
    fi
done

if [ -n "$VERSION_ARG" ]; then
    PACKAGE_VERSION="$VERSION_ARG"
    DEB_VERSION="${PACKAGE_VERSION}-1"
else
    BUILD_TIMESTAMP=$(date +%Y%m%d.%H%M%S)
    PACKAGE_VERSION="1.0.5~build.${BUILD_TIMESTAMP}"
    DEB_VERSION="${PACKAGE_VERSION}"
fi

ARCH=$(dpkg --print-architecture)
MAINTAINER="seb3773 <seb3773@github.com>"
DESCRIPTION="Classic-X Menu Kicker Applet (Standalone Plugin)"
BUILD_DIR="applet_package_build"
DEB_NAME="${PACKAGE_NAME}_${PACKAGE_VERSION}_${ARCH}.deb"

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
APPLET_SRC="$SCRIPT_DIR/src/applet"
BUILD_ROOT="$APPLET_SRC/build"

echo "Updating version header ($PACKAGE_VERSION)..."
cat <<EOF > "$APPLET_SRC/classicx_version.h"
#ifndef CLASSICX_VERSION_H
#define CLASSICX_VERSION_H

#define CLASSICX_VERSION "$PACKAGE_VERSION"

#endif // CLASSICX_VERSION_H
EOF

if [ -f "$SCRIPT_DIR/convert_images.py" ]; then
    echo "Updating embedded icons..."
    python3 "$SCRIPT_DIR/convert_images.py"
fi
if [ -f "$SCRIPT_DIR/convert_profiles.py" ]; then
    echo "Updating embedded profiles..."
    python3 "$SCRIPT_DIR/convert_profiles.py"
fi

echo "Building Classic-X Applet..."
mkdir -p "$BUILD_ROOT"
cd "$BUILD_ROOT"
cmake .. -DCMAKE_INSTALL_PREFIX=/opt/trinity
make -j$(nproc)

cd "$SCRIPT_DIR"

echo "Preparing package tree..."
rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR/opt/trinity/lib/trinity"
mkdir -p "$BUILD_DIR/opt/trinity/share/apps/kicker/applets"
mkdir -p "$BUILD_DIR/opt/trinity/share/icons/crystalsvg/64x64/apps"
mkdir -p "$BUILD_DIR/opt/trinity/share/icons/crystalsvg/scalable/apps"
mkdir -p "$BUILD_DIR/opt/trinity/share/icons/hicolor/64x64/apps"
mkdir -p "$BUILD_DIR/opt/trinity/share/icons/hicolor/scalable/apps"
mkdir -p "$BUILD_DIR/DEBIAN"

cp -a "$BUILD_ROOT/classicxapplet.so" "$BUILD_DIR/opt/trinity/lib/trinity/"
cp -a "$BUILD_ROOT/classicxapplet.la" "$BUILD_DIR/opt/trinity/lib/trinity/"
cp -a "$BUILD_ROOT/classicxapplet.desktop" "$BUILD_DIR/opt/trinity/share/apps/kicker/applets/"
cp -a "$SCRIPT_DIR/CX.png" "$BUILD_DIR/opt/trinity/share/icons/crystalsvg/64x64/apps/classicxapplet.png"
cp -a "$SCRIPT_DIR/CX.png" "$BUILD_DIR/opt/trinity/share/icons/crystalsvg/scalable/apps/classicxapplet.png"
cp -a "$SCRIPT_DIR/CX.png" "$BUILD_DIR/opt/trinity/share/icons/hicolor/64x64/apps/classicxapplet.png"
cp -a "$SCRIPT_DIR/CX.png" "$BUILD_DIR/opt/trinity/share/icons/hicolor/scalable/apps/classicxapplet.png"



echo "Stripping applet binary..."
if command -v sstrip >/dev/null 2>&1; then
    echo "Using sstrip for super-aggressive binary stripping..."
    sstrip "$BUILD_DIR/opt/trinity/lib/trinity/classicxapplet.so"
else
    strip --strip-unneeded "$BUILD_DIR/opt/trinity/lib/trinity/classicxapplet.so"
fi

cat <<EOF > "$BUILD_DIR/DEBIAN/postinst"
#!/bin/sh
set -e

# Automatically configure Classic-X APT repository for updates
if [ -d /etc/apt/sources.list.d ]; then
    cat << 'REPEOF' > /etc/apt/sources.list.d/classicx.list
# Classic-X Menu for Trinity Desktop (TDE) APT Repository
deb [trusted=yes] https://seb3773.github.io/ClassicX-tde/ stable main
REPEOF
fi

if [ -x /opt/trinity/bin/tdebuildsycoca ]; then
    /opt/trinity/bin/tdebuildsycoca >/dev/null 2>&1 || true
fi
exit 0
EOF
chmod 755 "$BUILD_DIR/DEBIAN/postinst"

cat <<EOF > "$BUILD_DIR/DEBIAN/postrm"
#!/bin/sh
set -e

if [ "\$1" = "purge" ] || [ "\$1" = "remove" ]; then
    rm -f /etc/apt/sources.list.d/classicx.list
fi

if [ -x /opt/trinity/bin/tdebuildsycoca ]; then
    /opt/trinity/bin/tdebuildsycoca >/dev/null 2>&1 || true
fi
exit 0
EOF
chmod 755 "$BUILD_DIR/DEBIAN/postrm"

cat <<EOF > "$BUILD_DIR/DEBIAN/control"
Package: $PACKAGE_NAME
Version: $DEB_VERSION
Section: x11
Priority: optional
Architecture: $ARCH
Depends: libtqt3-mt, tdelibs14-trinity, kicker-trinity
Maintainer: $MAINTAINER
Description: $DESCRIPTION
 Standalone Kicker applet providing the Classic-X Start Menu replacement with
 instant search filtering and sidebar buttons, without needing to patch tdebase.
EOF

dpkg-deb --build "$BUILD_DIR" "$DEB_NAME"
echo "Package created successfully: $DEB_NAME"
ls -lh "$DEB_NAME"

# Optional install test (pass --install / -i or set INSTALL_AFTER_BUILD=1)
if [ "$INSTALL_FLAG" = "1" ] || [ "$INSTALL_AFTER_BUILD" = "1" ]; then
    sudo apt install ./$DEB_NAME
    killall kicker && sleep 2 && kicker
fi
