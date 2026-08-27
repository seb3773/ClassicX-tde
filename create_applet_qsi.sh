#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
QSI_DIR="$SCRIPT_DIR/qsi_setup"
DEB_DIR="$QSI_DIR/deb_packages"
OUT_DIR="$QSI_DIR/output"

echo "=================================================="
echo " Classic-X Menu - Q4OS .qsi Installer Builder"
echo "=================================================="

# 1. Build or locate the latest .deb package
echo "Step 1: Building Debian .deb package..."
"$SCRIPT_DIR/create_applet_deb.sh" "$1"

# 2. Stage packages in qsi_setup
echo "Step 2: Staging files for Q4OS installer..."
mkdir -p "$DEB_DIR"
mkdir -p "$OUT_DIR"
rm -f "$DEB_DIR"/*.deb
rm -f "$OUT_DIR"/*.qsi

LATEST_DEB=$(ls -t "$SCRIPT_DIR"/tde-kicker-classicx-applet_*.deb 2>/dev/null | head -n 1)
if [ -z "$LATEST_DEB" ] || [ ! -f "$LATEST_DEB" ]; then
    echo "[Error] No .deb package found in $SCRIPT_DIR!"
    exit 1
fi

cp -a "$LATEST_DEB" "$DEB_DIR/"
echo "Staged package: $(basename "$LATEST_DEB")"

# Copy applet icon if available
if [ -f "$SCRIPT_DIR/CX.png" ]; then
    cp -a "$SCRIPT_DIR/CX.png" "$QSI_DIR/setup_templates/classicxapplet.png" 2>/dev/null || true
fi

# 3. Execute Q4OS build-qinstaller tool
echo "Step 3: Generating Q4OS .qsi installer..."
if ! command -v build-qinstaller >/dev/null 2>&1; then
    echo "[Error] 'build-qinstaller' command not found. Please install q4os-devpack-base."
    exit 1
fi

cd "$QSI_DIR"
BASE_VERSION="${1:-1.0.4}"

# Generate dynamic qinstaller with absolute paths to ensure deb packages are bundled
cat <<EOF > qinstaller
#***q4os*setup*config*header*do*not*delete*it***#
PK_NAME="tde-kicker-classicx-applet"
APPNAME_DESC="Classic-X Menu for Trinity Desktop"
APP_ICON="classicxapplet"
PK_VERS="$BASE_VERSION"
SETUP_TYPE="2"
INST_DEBS="tde-kicker-classicx-applet"
DEBPCKS_DIR="$DEB_DIR"
TEMPLATES_DIR="$QSI_DIR/setup_templates"
OUT_DIR="$OUT_DIR"
APPLNK_ENTRY="0"
DESKTOP_ENTRY="0"
MENU_ENTRY="0"
DSTR_BASE="debian;ubuntu"
DSTR_EDTN="bullseye;bookworm;trixie;jammy;noble"
Q4VER_MIN="4.0"
EOF

build-qinstaller qinstaller

cd "$SCRIPT_DIR"
LATEST_QSI=$(ls -t "$OUT_DIR"/*.qsi 2>/dev/null | head -n 1)
if [ -n "$LATEST_QSI" ] && [ -f "$LATEST_QSI" ]; then
    FINAL_QSI_NAME=$(basename "$LATEST_QSI")
    cp -a "$LATEST_QSI" "$SCRIPT_DIR/$FINAL_QSI_NAME"
    chmod +x "$SCRIPT_DIR/$FINAL_QSI_NAME"
    echo ""
    echo "=================================================="
    echo " SUCCESS: Q4OS Installer generated successfully!"
    echo " File: $SCRIPT_DIR/$FINAL_QSI_NAME"
    echo " Size: $(ls -lh "$SCRIPT_DIR/$FINAL_QSI_NAME" | awk '{print $5}')"
    echo "=================================================="
else
    echo "[Error] Failed to generate .qsi installer."
    exit 1
fi
