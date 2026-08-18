# TDE Kicker Menu Classic-X (Standalone Applet)

A modern, high-performance redesign of the Trinity Desktop Environment (TDE) Classic Menu (KMenu), built as a **standalone Kicker panel applet plugin** (`classicxapplet.so`).

Classic-X combines the lightning-fast responsiveness of classic TDE Kmenu with modern UX features: **Windows 10-style instant search filtering**, **typo-tolerant fuzzy suggestions**, **an interactive quick-access sidebar**, **extensive visual theming (custom banners, 3-part header pictures, dynamic text overlays)**, and **several built-in preset profiles**.

---

## Table of Contents
1. [Key Features Overview](#key-features-overview)
2. [Interactive Navigation & Search Guide (Keyboard & Mouse UX)](#interactive-navigation--search-guide-keyboard--mouse-ux)
3. [Settings & Customization Reference](#settings--customization-reference)
   - [Built-in & User Profiles](#1-built-in--user-profiles)
   - [General Settings & Application History](#2-general-settings--application-history)
   - [Colors, Transparency & Typography](#3-colors-transparency--typography)
   - [Interactive Sidebar & Power Management](#4-interactive-sidebar--power-management)
   - [Start Icon & UI Icons Customization](#5-start-icon--ui-icons-customization)
   - [Sidebar Picture & Banner Themes](#6-sidebar-picture--banner-themes)
   - [Top Header Picture & Dynamic Text Overlay](#7-top-header-picture--dynamic-text-overlay)
4. [Build, Packaging & Installation](#build-packaging--installation)
5. [Technical Architecture & Low-Level C++ Optimizations](#technical-architecture--low-level-c-optimizations)
6. [Screenshots](#screenshots)

---

## Key Features Overview

* **Instant Type-to-Search**: Start typing any alphanumeric key as soon as the menu opens to immediately enter search mode — no dedicated search bar click required.
* **Smart Fuzzy Search & Accent Normalization**: Handles typos, omissions, and transpositions via an L1-cache optimized Damerau-Levenshtein engine. Automatically normalizes multilingual diacritics (`é`, `è`, `ê`, `à`, `î`, `ö`, `ç` $\rightarrow$ `e`, `a`, `i`, `o`, `c`).
* **Recent-Prioritized Ranking**: Search results matching applications in your recent launch history (`kickerrc`) are automatically promoted to top positions.
* **Modern Quick-Access Sidebar**: Dedicated interactive buttons for **User / Session**, **Documents**, **Pictures**, **Settings**, and **Log Out** with optional hover-triggered submenus.
* **Dynamic 3-Part Header Banner & Multi-Element Text Overlay**: 3-part header images (Left/Center/Right) with customizable themes and dynamic text displaying User Name, Custom Text, Localized Date, and Localized Time (HH:MM).
* **20 Built-in Preset Profiles**: Instant visual transformations (e.g. *2001*, *AlienX*, *BlackMac*, *C64*, *DebianDevil*, *Q4OSmodern*, *Trinity*, *WinX*) embedded as ultra-compact zero-relocation bytecode (< 3.0 KB memory footprint).
* **Deep "Show in Tree" Navigation**: Right-click any search hit to instantly restore the full application category tree and highlight that specific program inside its folder.
* **Standalone Plugin**: Zero modifications required to the system `kicker` binary or `tdebase` packages.

---

## Interactive Navigation & Search Guide (Keyboard & Mouse UX)

Classic-X provides a fluid, friction-free keyboard and mouse workflow:

```
[Open Menu] ─── (Type letters) ───► [Instant Search Results]
    │                                     │
    │ (ESC / Backspace on empty query)    │ (Enter on single match OR highlighted item)
    ▼                                     ▼
[Full Category Tree]                [Launch Application]
    │
    │ (Right-click item)
    ▼
[Context Menu: Add to Desktop / Add to Panel / Show in Tree]
```

### 1. Type-to-Search & Instant Filtering
* **Immediate Activation**: Simply open the menu (via Super key, panel button, or shortcut) and start typing (e.g., `term`). The menu automatically hides the category tree and displays matching applications in real-time.
* **Single-Match Auto-Launch**: When your query narrows down to a **single exact match**, pressing **Enter** launches it immediately without requiring manual arrow selection.
* **Highlighted Launch**: If multiple results are displayed, use **Up / Down Arrow keys** to highlight a candidate and press **Enter** to launch.

### 2. Seamless Tree Stash & Navigation Keys
* **Backspace Navigation**:
  * While editing a search query, **Backspace** deletes characters normally.  
* **Escape (ESC) Handling**:
  * **First ESC**: Exits search mode, clears the query, and restores the full category tree view.
  * **Second ESC** (or ESC while viewing the category tree): Closes the menu.
* **Left / Right / Down Arrows**: Navigate across submenus, recent applications, and sidebar items seamlessly.

### 3. Smart Fuzzy Search (*Did you mean...*)
* If an exact query yields 0 matches, Classic-X automatically triggers its typo-tolerant suggestion engine.
* A title header displays *"No exact results found. Did you mean:"*.
* Suggestions evaluate prefix distances and Damerau-Levenshtein edit distances (e.g. typing `flz` suggests **FileZilla**; typing `gmp` suggests **GIMP**; typing `calc` suggests **KCalc**).
* Exact Enter auto-launch is safely disabled on fuzzy suggestions to prevent accidental launches.

### 4. Right-Click Context Menu & "Show in Tree"
* Right-clicking any application entry (in the search results, recent apps, or category tree) opens the action context menu:
  * **Add to Desktop**: Places a desktop shortcut icon.
  * **Add to Main Panel**: Adds a quick-launch button to the Kicker panel (automatically disabled if the panel is locked in `kickerrc`).
  * **Show in Tree**: Exits search mode, restores the main hierarchy, and recursively expands the exact submenu folder containing the application, scrolling to and highlighting the target item.

---

## Settings & Customization Reference

Access the settings dialog by right-clicking the applet handle or menu button and selecting **Menu Settings**.

### 1. Built-in & User Profiles
* **Preset Profiles Selector**: Switch between 20 crafted built-in profiles (*2001*, *A520ST*, *AlienX*, *BlackMac*, *BlueWork*, *C64*, *DebianDevil*, *GoldFlower*, *GreenWin*, *Japan*, *Q4OSaqua*, *Q4OSmodern*, *RocketScience*, *System7*, *ThinBlack*, *Trinity*, *WinX*, *WintNT2K*, *Woody*, *X11minimal*).
* **Custom Profiles**: Save your current layout, colors, icons, and picture configuration under a custom name, or delete obsolete user profiles saved in `~/.trinity/share/apps/classicxapplet/profiles/`.

### 2. General Settings & Application History
* **Always Show Search Bar**: Choose between a permanently visible search input field (on-demand type-to-search is always activated independently of this choice).
* **Menu Entry Format**: Choose how applications are named across the menu:
  * *Name only* (e.g., `Konqueror`)
  * *Description only* (e.g., `Web Browser`)
  * *Name (Description)* (e.g., `Konqueror (Web Browser)`)
  * *Description (Name)* (e.g., `Web Browser (Konqueror)`)
* **Tree Icon Size**: Select application icon rendering size (Small `16×16`, Classic `20×20`, `22×22`, `24×24`, `32×32`).
* **Search & Recent Limits**:
  * *Max search results*: 4 to 20 visible items.
  * *Max recent documents*: 5 to 30 items.
* **Application History Mode** (*Mutually Exclusive*):
  * **Display recently used applications**: Shows the most recently launched apps at the top of the menu + Item count spinbox (2 to 6 items).
  * **Display most used applications**: Shows the most frequently launched apps based on historical launch counts + Item count spinbox (2 to 6 items).
  * **Disabled**: Completely removes the top history section.
* **Standard TDE Menu Toggles**: Individually show/hide Trinity Control Center, Run Command, Bookmarks, Print System, Quick-Browser, Network Folders, System Menu, Recent Documents, Special User Menu, and Special Shutdown Menu.

### 3. Colors, Transparency & Typography
* **Transparency Slider**: Adjust menu background opacity from 0% (opaque) to 80% transparency.
* **Color Modes**:
  * *System Default*: Default color theme
  * *TDE*: Follows active TDE widget color scheme
  * *Custom Colors*: Independent color pickers with real-time swatch preview for:
    * **Background Color**
    * **Sidebar Background Color**
    * **Text Area Background Color**
    * **Title Banner Foreground & Background Colors**
    * **Search Text Input Color**
    * **Button Hover Highlight Color**
* **Font Mode**: Use standard TDE System font (`TDEGlobalSettings::generalFont()`) or select a custom typography font and size via font dialog.

### 4. Interactive Sidebar & Power Management
* **Sidebar Toggle & Geometry**: Enable/disable sidebar, configure custom width (24px to 64px), and choose vertical button alignment (*Top*, *Center*, *Bottom*).
* **Hover Activation**: Submenus (User / Shutdown) can open automatically on pointer hover with configurable delay (100ms to 1000ms).
* **Quick-Access Buttons**: Toggle individual sidebar buttons for **User Menu**, **Shutdown Menu**, **Settings**, **Documents**, and **Pictures**.
* **Submenu Height Mode**: Choose between **Full Menu Height** (submenus expand to match main menu height) or **Custom Height** (150px to 600px).
* **Power Management Actions**: Toggle individual power actions (**Power Off**, **Reboot**, **Suspend**, **Hybrid Suspend**, **Hibernate**) with automatic backend detection (ConsoleKit, systemd-logind, UPower, TDE PowerManager).

### 5. Start Icon & UI Icons Customization
* **Start Menu Button**:
  * *Source*: Embedded presets (*Classic, Modern, WinBlue, Tux, Debian, Commodore, Apple, Atari, Q4OS...*), TDE System Icon, or Custom file path.
  * *Rendering*: **Full-scale** (fills panel button height), **Invert colors**, and **Colorize tint** with custom color picker.
* **UI Icons (Sidebar & Submenu Chrome)**:
  * *Size*: 16px, 22px, 24px, 28px, 32px, 36px, 48px.
  * *Source Customization*: Assign individual icon sources or apply batch presets across all 9 UI categories (Settings, Documents, Pictures, User, Shutdown, Power Off, Reboot, Suspend, Hibernate).
  * *Filters*: **Invert colors** and **Colorize tint**.

### 6. Sidebar Picture & Banner Themes
* **Modes**: Disabled, Embedded Theme (*Ocean, Lava, HAL9000, Borders, Carbon, Waves, Retro...*), or Custom Image file.
* **Layout & Alignment**: Align Top / Bottom, Stretch / Crop to sidebar width, Extend beyond edges.
* **Color Tint**: Optional colorize filter for sidebar pictures.

### 7. Top Header Picture & Dynamic Text Overlay
* **3-Part Header Banner**: Embed Left/Center/Right composite banners (*Royal, Slate, Ocean, Minimal, Classic...*) or custom image.
* **Dynamic Multi-Element Text Overlay**:
  * Combine **User Name**, **Custom Text** (e.g. *"Trinity Desktop"*), **Localized Date**, and **Localized Time** (HH:MM without seconds).
  * Automatically formats with smart `" - "` separators.
  * Text Color control: *TDE Default*, *Title Text Color*, or *Custom Color*.
  * Optional colorize filter on header graphics.

---

## Build, Packaging & Installation

Classic-X is distributed as a standalone Debian package that installs cleanly into Trinity's directory tree (`/opt/trinity/`).

### Prerequisites
* TDE development headers: `tdebase-trinity-dev`, `tqca-trinity-dev`, `libtqt3-mt-dev`
* Build tools: `cmake` (>= 3.0), `g++`, `make`, `dpkg-deb`

### Building the Package

Run the build script from the repository root:

```bash
./create_applet_deb.sh
```

This automated script will:
1. Run `convert_images.py` and `convert_profiles.py` to regenerate embedded assets.
2. Configure CMake with optimal C++ flags and build `classicxapplet.so`.
3. Strip the binary aggressively (`sstrip` / `strip`).
4. Generate the `.deb` package (`tde-kicker-classicx-applet_<version>_amd64.deb`).

### Installation & Activation

1. Install the generated Debian package:
   ```bash
   sudo apt install ./tde-kicker-classicx-applet_*_amd64.deb
   ```

2. Restart Kicker to reload plugins:
   ```bash
   kicker --replace &
   ```

3. Right-click the Kicker panel $\rightarrow$ **Add Applet to Panel...** $\rightarrow$ Select **Classic-X Menu**.

---

## Technical Architecture & Low-Level C++ Optimizations

| Component / File | Purpose |
|------------------|---------|
| `src/applet/classicx_applet.cpp` | Applet entry point and Kicker panel button integration |
| `src/applet/k_mnu.cpp` | Main menu class (`PanelKMenu`), search engine, result layout, tree stash / restore |
| `src/applet/classicx_settings_dialog.cpp` | Settings dialog (`ClassicXSettingsDialog`) |
| `src/applet/classicx_profile.cpp` | Profile management (unified built-in & disk profiles) |
| `src/applet/service_mnu.cpp` | Service menu class (`PanelServiceMenu`), context menu, Show in Tree, `kickerrc` lock check |
| `src/applet/global.cpp` | Shared helpers: `treeIconPixelSize()`, `menuIconSet()`, search pad icon cache |
| `src/applet/recentapps.cpp` | History manager (`RecentlyLaunchedApps`), `kickerrc` `RecentAppsStat` reader/writer |
| `src/applet/classicxSettings.kcfg` | Configuration skeleton (`MenuEntryHeight`, `ShowRecentApps`, `NumRecentApps`, `SidebarHoverDelay`, etc.) |
| `convert_profiles.py` | Generator for zero-relocation binary delta profile database (`classicx_builtin_profiles.h`) |
| `create_applet_deb.sh` | Automated build and packaging script for Debian/TDE |

### Load-Bearing TQt3 Framework Constraints

TQt3 `TQPopupMenu` and `TQIconSet` have specific internal constraints that must be preserved:

1. **One Pixel Size `S` for Tree Application Icons**:
   * Tree/search application icons use `KickerLib::treeIconPixelSize()`.
   * `UiIconSize` is reserved for sidebar/chrome only. `menuIconSet()` scales icons to exact $S \times S$ dimensions and publishes pixmaps across Small and Large modes.
2. **Constant Search Height via Transparent Pads**:
   * Search results allocate `maxSearchResults` rows using real hits + transparent pad items (`treeIconPadIconSet`), followed by `adjustSize()`.
   * TQt3 downscales large pads if not explicitly assigned matching sizes, causing height jitter. Cached pads guarantee identical row height whether 1 or 20 results match.
3. **Tree Stash (Instant Search Exit)**:
   * On first keystroke, tree items are hidden in place (`setItemVisible(false)`), preserving item IDs and submenu instances.
   * Search rows use disjoint ID ranges (`6000+`) to avoid collisions with tree IDs (`4242–5242`).
   * When exiting search (Backspace/ESC), search rows are removed and tree items unhidden in $O(1)$ without rebuilding Sycoca or bookmarks.

### Performance & Memory Optimizations

1. **L1-Cache Compact Damerau-Levenshtein Engine**:
   * DP matrix is compressed from $65 \times 65$ (16.9 KB) to a $3 \times 65$ circular row buffer (780 bytes), remaining 100% inside CPU L1 Data Cache.
   * Prefix edit distances are computed in a single pass directly from row `len1` in $O(1)$ time.
   * **Ukkonen Early Exit**: Halts computation immediately when row minimums exceed the error threshold.
2. **Pre-Tokenized Sycoca Search Index**:
   * App names and descriptions are tokenized during Sycoca build (`buildServiceCache`). Interactive typing operates on constant references (`const TQStringList &`) with zero runtime regex evaluations.
3. **Single-Pass UTF-16 Normalization**:
   * Combines lowercase conversion and diacritics stripping (`toLowerAndUnaccent`) in a single linear pass over raw UTF-16 pointers (`TQChar *`), eliminating heap allocations and copy-on-write detachments.
4. **Contiguous Vector Memory Storage**:
   * `RecentlyLaunchedApps` uses contiguous `TQValueVector` storage instead of node-allocated linked lists, maximizing L1/L2 cache prefetching.
5. **Non-Blocking Display Manager IPC**:
   * Session control (`dmctl.cpp`) uses non-blocking POSIX sockets with `fcntl(O_NONBLOCK)` and bounded `poll()` timeouts (~2000ms), preventing UI lockups if display daemons freeze.
6. **Zero-Relocation Delta Built-in Profiles Architecture**:
   * In 64-bit shared modules (`.so`), static struct arrays with string pointers generate hundreds of 24-byte dynamic relocations (`.rela.dyn`). Classic-X encodes all 20 built-in profiles as a contiguous byte stream (`s_profileDeltaBlob[]`), eliminating 100% of relocation overhead.
   * A 150-byte baseline defines defaults; each profile encodes only differential keys (3-byte RGB colors, 1-2 byte scalars, inline UTF-8).
   * Inactive options are pruned during build, keeping the entire 20-profile database under **3.0 KB in memory**.
7. **Selective Granular `-Os` Compilation**:
   * The settings dialog (`classicx_settings_dialog.cpp`) and profile loader (`classicx_profile.cpp`) are compiled with `-Os` (`#pragma GCC optimize ("Os")` + CMake `COMPILE_FLAGS "-Os"`), reducing binary size by ~16 KB while keeping core search and indexing engines at `-O2 -flto=auto` performance.

---

## Screenshots

![good old classic menu](screenshots/screenshot_kicker_menu_classic-x_A.jpg)  
![instant search](screenshots/screenshot_kicker_menu_classic-x_B.jpg)  
![configurable](screenshots/screenshot_kicker_menu_classic-x_C.jpg)  
![sidebar buttons](screenshots/screenshot_kicker_menu_classic-x_D.jpg)  
