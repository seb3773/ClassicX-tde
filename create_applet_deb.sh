#!/bin/bash
set -e

PACKAGE_NAME="tde-kicker-classicx-applet"
BASE_VERSION="${1:-1.0.0}"
BUILD_TIMESTAMP=$(date +%Y%m%d.%H%M%S)

if [ -n "$1" ]; then
    PACKAGE_VERSION="$1"
    DEB_VERSION="${PACKAGE_VERSION}-1"
else
    PACKAGE_VERSION="${BASE_VERSION}~build.${BUILD_TIMESTAMP}"
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
if [ -x /opt/trinity/bin/tdebuildsycoca ]; then
    /opt/trinity/bin/tdebuildsycoca >/dev/null 2>&1 || true
fi
exit 0
EOF
chmod 755 "$BUILD_DIR/DEBIAN/postinst"

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

# Optional install test (set INSTALL_AFTER_BUILD=1 or pass --install as 2nd arg)
if [ "$INSTALL_AFTER_BUILD" = "1" ] || [ "$2" = "--install" ]; then
    sudo apt install ./$DEB_NAME
    killall kicker && sleep 2 && kicker
fi
