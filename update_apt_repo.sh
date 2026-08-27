#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_DIR="$SCRIPT_DIR"
PAGES_DIR="/tmp/classicx_gh_pages_$$"

echo "=================================================="
echo " Classic-X - APT Repository & GitHub Pages Sync"
echo "=================================================="

# Check tools
for cmd in git dpkg-scanpackages gzip apt-ftparchive; do
    if ! command -v "$cmd" >/dev/null 2>&1; then
        echo "[Error] Required command '$cmd' not found."
        exit 1
    fi
done

# Ensure we have at least one deb package
DEB_FILES=($(ls -t "$REPO_DIR"/tde-kicker-classicx-applet_*.deb 2>/dev/null || true))
if [ ${#DEB_FILES[@]} -eq 0 ]; then
    echo "[Info] No .deb package found in root. Building latest package..."
    "$REPO_DIR/create_applet_deb.sh"
    DEB_FILES=($(ls -t "$REPO_DIR"/tde-kicker-classicx-applet_*.deb 2>/dev/null || true))
fi

if [ ${#DEB_FILES[@]} -eq 0 ]; then
    echo "[Error] Failed to locate or build .deb package!"
    exit 1
fi

echo "Staging packages..."
rm -rf "$PAGES_DIR"
mkdir -p "$PAGES_DIR"

REMOTE_URL="$(cd "$REPO_DIR" && git remote get-url origin)"
REMOTE_PAGES=$(git ls-remote --heads origin gh-pages 2>/dev/null || true)
if [ -n "$REMOTE_PAGES" ]; then
    echo "Cloning existing gh-pages branch from remote..."
    git clone --single-branch --branch gh-pages "$REMOTE_URL" "$PAGES_DIR"
else
    echo "Initializing new gh-pages branch..."
    cd "$PAGES_DIR"
    git init
    git checkout -b gh-pages
    git remote add origin "$REMOTE_URL"
fi

cd "$PAGES_DIR"

# Create standard APT pool and dists structure
POOL_DIR="$PAGES_DIR/pool/main/t/tde-kicker-classicx-applet"
DISTS_DIR="$PAGES_DIR/dists/stable/main/binary-amd64"
mkdir -p "$POOL_DIR"
mkdir -p "$DISTS_DIR"

# Copy all .deb packages from root
for deb in "${DEB_FILES[@]}"; do
    cp -u "$deb" "$POOL_DIR/" 2>/dev/null || cp -a "$deb" "$POOL_DIR/"
    echo "  Added: $(basename "$deb")"
done

# Also copy latest .qsi if present
QSI_FILES=($(ls -t "$REPO_DIR"/setup_tde-kicker-classicx-applet_*.qsi 2>/dev/null || true))
if [ ${#QSI_FILES[@]} -gt 0 ]; then
    for qsi in "${QSI_FILES[@]}"; do
        cp -u "$qsi" "$PAGES_DIR/" 2>/dev/null || cp -a "$qsi" "$PAGES_DIR/"
        echo "  Added QSI: $(basename "$qsi")"
    done
fi

# Generate Packages & Packages.gz
echo "Generating Packages index..."
dpkg-scanpackages --multiversion pool/ /dev/null > "$DISTS_DIR/Packages"
gzip -9c "$DISTS_DIR/Packages" > "$DISTS_DIR/Packages.gz"

# Generate Release file
echo "Generating Release manifest..."
apt-ftparchive \
  -o APT::FTPArchive::Release::Origin="ClassicX" \
  -o APT::FTPArchive::Release::Label="Classic-X TDE Repository" \
  -o APT::FTPArchive::Release::Suite="stable" \
  -o APT::FTPArchive::Release::Codename="stable" \
  -o APT::FTPArchive::Release::Architectures="amd64" \
  -o APT::FTPArchive::Release::Components="main" \
  -o APT::FTPArchive::Release::Description="APT Repository for Classic-X Trinity Desktop (TDE) Applet" \
  release "$PAGES_DIR/dists/stable" > "$PAGES_DIR/dists/stable/Release"

# Copy assets (logo, etc.)
if [ -f "$REPO_DIR/about.png" ]; then
    cp -a "$REPO_DIR/about.png" "$PAGES_DIR/"
fi
if [ -f "$REPO_DIR/CX.png" ]; then
    cp -a "$REPO_DIR/CX.png" "$PAGES_DIR/"
fi

# Create .nojekyll to prevent GitHub Pages Jekyll processing
touch "$PAGES_DIR/.nojekyll"

# Find latest file names for HTML download buttons
LATEST_DEB_NAME=$(basename "${DEB_FILES[0]}")
LATEST_VERSION=$(echo "$LATEST_DEB_NAME" | sed -n 's/.*classicx-applet_\([^_]*\)_.*/\1/p')
if [ -z "$LATEST_VERSION" ]; then
    LATEST_VERSION="1.0.4"
fi

LATEST_QSI_NAME=""
if [ ${#QSI_FILES[@]} -gt 0 ]; then
    LATEST_QSI_NAME=$(basename "${QSI_FILES[0]}")
fi

cat << EOF > "$PAGES_DIR/index.html"
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Classic-X TDE v${LATEST_VERSION} - APT Repository</title>
  <link rel="icon" type="image/png" href="CX.png">
  <style>
    :root {
      --bg: #12141a;
      --card-bg: #1c1f2b;
      --accent: #3a86ff;
      --accent-grad: linear-gradient(135deg, #3a86ff, #00f2fe);
      --text: #e2e8f0;
      --text-muted: #94a3b8;
      --code-bg: #0f1117;
      --border: #2e364f;
    }
    * { box-sizing: border-box; margin: 0; padding: 0; }
    body {
      font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif;
      background-color: var(--bg);
      color: var(--text);
      line-height: 1.6;
      padding: 40px 20px;
    }
    .container {
      max-width: 800px;
      margin: 0 auto;
    }
    header {
      text-align: center;
      margin-bottom: 40px;
    }
    .logo {
      width: 96px;
      height: 96px;
      margin-bottom: 16px;
      filter: drop-shadow(0 8px 24px rgba(58, 134, 255, 0.4));
      border-radius: 50%;
      transition: transform 0.3s cubic-bezier(0.34, 1.56, 0.64, 1);
    }
    .logo:hover {
      transform: scale(1.1) rotate(4deg);
    }
    .badge {
      display: inline-block;
      padding: 4px 12px;
      font-size: 0.85rem;
      font-weight: 600;
      color: #fff;
      background: var(--accent-grad);
      border-radius: 20px;
      margin-bottom: 12px;
      text-transform: uppercase;
      letter-spacing: 0.5px;
    }
    .version-pill {
      display: inline-block;
      font-size: 1.1rem;
      font-weight: 600;
      color: #38bdf8;
      background: rgba(56, 189, 248, 0.12);
      border: 1px solid rgba(56, 189, 248, 0.35);
      padding: 2px 12px;
      border-radius: 20px;
      vertical-align: middle;
      margin-left: 8px;
    }
    h1 {
      font-size: 2.2rem;
      font-weight: 700;
      margin-bottom: 8px;
    }
    p.lead {
      font-size: 1.1rem;
      color: var(--text-muted);
    }
    .card {
      background: var(--card-bg);
      border: 1px solid var(--border);
      border-radius: 12px;
      padding: 24px;
      margin-bottom: 24px;
      box-shadow: 0 10px 30px rgba(0, 0, 0, 0.3);
    }
    h2 {
      font-size: 1.3rem;
      margin-bottom: 16px;
      display: flex;
      align-items: center;
      gap: 10px;
      color: #fff;
    }
    pre {
      background: var(--code-bg);
      border: 1px solid var(--border);
      border-radius: 8px;
      padding: 16px;
      overflow-x: auto;
      font-family: "Courier New", Courier, monospace;
      font-size: 0.95rem;
      color: #38bdf8;
      margin-bottom: 12px;
    }
    .features-grid {
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(240px, 1fr));
      gap: 16px;
      margin-top: 14px;
    }
    .feature-item {
      background: var(--code-bg);
      border: 1px solid var(--border);
      border-radius: 8px;
      padding: 14px 16px;
    }
    .feature-item h3 {
      font-size: 1rem;
      color: #38bdf8;
      margin-bottom: 4px;
    }
    .feature-item p {
      font-size: 0.88rem;
      color: var(--text-muted);
      line-height: 1.5;
    }
    .btn-group {
      display: flex;
      flex-wrap: wrap;
      gap: 12px;
      margin-top: 16px;
    }
    .btn {
      display: inline-flex;
      align-items: center;
      gap: 8px;
      padding: 10px 20px;
      border-radius: 8px;
      text-decoration: none;
      font-weight: 600;
      font-size: 0.95rem;
      transition: all 0.2s ease;
    }
    .btn-primary {
      background: var(--accent-grad);
      color: #fff;
    }
    .btn-primary:hover {
      opacity: 0.9;
      transform: translateY(-2px);
    }
    .btn-secondary {
      background: var(--code-bg);
      color: var(--text);
      border: 1px solid var(--border);
    }
    .btn-secondary:hover {
      background: #1e2230;
      transform: translateY(-2px);
    }
    footer {
      text-align: center;
      margin-top: 40px;
      font-size: 0.9rem;
      color: var(--text-muted);
    }
    footer a { color: var(--accent); text-decoration: none; }
  </style>
</head>
<body>
  <div class="container">
    <header>
      <img src="about.png" alt="Classic-X Logo" class="logo">
      <br>
      <div class="badge">Official APT Repository • v${LATEST_VERSION}</div>
      <h1>Classic-X Menu for Trinity (TDE) <span class="version-pill">v${LATEST_VERSION}</span></h1>
      <p class="lead">Automatic updates for Debian, Q4OS, and Ubuntu-based TDE systems.</p>
    </header>

    <div class="card">
      <h2>🚀 Method 1: Add the APT Repository (Recommended)</h2>
      <p style="margin-bottom: 12px; color: var(--text-muted);">
        Add the official Classic-X repository to receive automated updates alongside your system:
      </p>
      <pre><code>echo "deb [trusted=yes] https://seb3773.github.io/ClassicX-tde/ stable main" | sudo tee /etc/apt/sources.list.d/classicx.list
sudo apt update
sudo apt install tde-kicker-classicx-applet</code></pre>
    </div>

    <div class="card">
      <h2>📦 Method 2: Direct Package Downloads (v${LATEST_VERSION})</h2>
      <p style="color: var(--text-muted); margin-bottom: 12px;">
        Download the standalone installer or Debian package directly:
      </p>
      <div class="btn-group">
        ${LATEST_QSI_NAME:+<a class="btn btn-primary" href="${LATEST_QSI_NAME}">📥 Download Q4OS Installer v${LATEST_VERSION} (.qsi)</a>}
        <a class="btn btn-secondary" href="pool/main/t/tde-kicker-classicx-applet/${LATEST_DEB_NAME}">📦 Download Debian Package v${LATEST_VERSION} (.deb)</a>
      </div>
    </div>

    <div class="card">
      <h2>✨ Key Features</h2>
      <div class="features-grid">
        <div class="feature-item">
          <h3>⚡ Instant Type-to-Search</h3>
          <p>Start typing anywhere to filter applications instantly. Features typo-tolerant fuzzy suggestions and multilingual accent normalization.</p>
        </div>
        <div class="feature-item">
          <h3>🎨 23 Built-in Profiles</h3>
          <p>Instant visual transformations embedded as ultra-compact bytecode (&lt; 3.7 KB). Custom palettes, transparency, and font support.</p>
        </div>
        <div class="feature-item">
          <h3>🖼️ 3-Part Header Banner</h3>
          <p>Composite Left/Center/Right graphics with live overlays: User Name, Custom text, Free RAM probe (GB), Date, and Time (HH:MM).</p>
        </div>
        <div class="feature-item">
          <h3>📌 Quick-Access Sidebar</h3>
          <p>Quick access to User, Shutdown, Documents, Pictures, Downloads, and Settings with configurable width and hover-triggered submenus.</p>
        </div>
        <div class="feature-item">
          <h3>🎬 Smooth Opening Animation</h3>
          <p>Fluid, tear-free window sliding animation with sub-millisecond geometry calculation adapting to panel position and screen edges.</p>
        </div>
        <div class="feature-item">
          <h3>🚀 Zero-Lag C++ Architecture</h3>
          <p>Standalone Kicker applet plugin with L1-cache optimized algorithms, sub-microsecond kernel probes, and zero external runtime dependencies.</p>
        </div>
      </div>
    </div>

    <footer>
      <p>Source Code &amp; Releases: <a href="https://github.com/seb3773/ClassicX-tde" target="_blank">github.com/seb3773/ClassicX-tde</a></p>
      <p style="margin-top: 6px;">Developed with ❤️ for the Trinity Desktop Environment community.</p>
    </footer>
  </div>
</body>
</html>
EOF

# Git commit and push to gh-pages
echo "Committing and pushing to gh-pages branch..."
git add -A
git commit -m "Update APT repository and packages: $(date +'%Y-%m-%d %H:%M:%S')" || echo "No changes to commit."
git push origin gh-pages

echo "Cleaning up temporary directory..."
rm -rf "$PAGES_DIR"

echo "=================================================="
echo " SUCCESS: APT repository updated on gh-pages!"
echo " URL: https://seb3773.github.io/ClassicX-tde/"
echo "=================================================="
