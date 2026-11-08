# How "Note" Was Built — Reproduction Guide

This document records the conversation and steps that led to building **Note**, a
lightweight text editor for the Omarchy ecosystem, modeled on Linux Mint's
`xed`. It is written so the process can be reproduced on another (more powerful)
machine.

---

## 1. Starting Point

The user was exploring the Omarchy ecosystem and asked two questions:

1. "What is Strata in terms of the Omarchy ecosystem?"
   - Answer: Strata is a Rust + GTK4 keyboard-first file manager designed for
     Omarchy (github.com/lgse/strata). Not installed locally.

2. "Is it possible to get something like this for Omarchy? github.com/linuxmint/xed"
   - `xed` is Linux Mint's small GTK3 text editor (package `extra/xed` on Arch).
   - It is installable on Omarchy via `sudo pacman -S xed`, but is GTK3/XWayland,
     not theme-native.

The user then asked: "Could you make it into an app that fits into the Omarchy
ecosystem?" and clarified: "based on xed's functions and behaviour."

---

## 2. Approach Decisions

- **Scope**: Build a native editor capturing xed's core behaviors: tabbed
  editing, syntax highlighting, line numbers, find/replace, word wrap, zoom,
  go-to-line, auto-indent, atomic saves, unsaved-changes prompts, theme
  following.
- **Stack**: Qt 6 Widgets + qmake6 (not the omarchy-app skill's Qt Quick path,
  because a text editor's editing surface is best served by QPlainTextEdit).
  Still follows Omarchy conventions: live theme following, `.desktop` + icon,
  Arch package via `makepkg`, keyboard-first.
- **Name**: `note` (binary, package, app id, desktop file, icon all share it).
- **Location**: `/home/varo/Work/note`, git-initialized.

### Environment (original machine)

- Omarchy 4.5.0-3, Qt 6.12.0, qmake6, make, g++, Arch Linux.
- Language set to German (`de`), theme accent `#89b4fa` (Catppuccin-ish dark).

---

## 3. Project Skeleton

```
note/
├── note.pro
├── bin/build
├── bin/install
├── src/
│   ├── main.cpp
│   ├── mainwindow.h / .cpp
│   ├── editor.h / .cpp          (QPlainTextEdit + line numbers + highlighter)
│   ├── highlighter.h / .cpp     (multi-language syntax highlighter)
│   ├── theme.h / .cpp           (live Omarchy accent/radius follower)
│   ├── findreplace.h / .cpp     (find/replace dialog)
│   └── resources.qrc
├── pkgbuild/
│   ├── PKGBUILD
│   ├── note.desktop
│   └── note.svg
├── README.md
├── LICENSE
└── .gitignore
```

---

## 4. Key Implementation Notes

### Theme following (`src/theme.h/.cpp`)

- Reads `~/.local/state/omarchy/current/theme/colors.toml` for `accent`,
  `background`, `foreground`, `selection`, `muted`.
- Reads Hyprland rounding via `hyprctl getoption decoration:rounding -j`.
- Falls back to accent `#FFD60A` when Omarchy is absent.
- Watches the file/directory with `QFileSystemWatcher` and re-arms on change
  (theme switches replace files/symlinks), plus a 2s polling fallback.
- Emits `changed()` signal.

### Editor (`src/editor.h/.cpp`)

- `QPlainTextEdit` subclass with a `LineNumberArea` widget.
- `Highlighter` attached to the document.
- Tab key inserts 4 spaces; Enter auto-indents based on previous line and
  increases indent after `{`, `:`, `(`.
- Configurable current-line highlight color.

### Highlighter (`src/highlighter.h/.cpp`)

- `QSyntaxHighlighter` with keyword/type lists per language, common rules for
  numbers, strings, comments (line + block), preprocessor.
- Language selected by file extension via `MainWindow::detectLanguage`.

### Main window (`src/mainwindow.h/.cpp`)

- Tabbed (`QTabWidget`), closable/movable tabs.
- Menus: File, Edit, View, Help with standard shortcuts.
- Status bar: `Ln X, Col Y | language | UTF-8`.
- Shortcuts: Ctrl+N new, Ctrl+O open, Ctrl+S save, Ctrl+Shift+S save as,
  Ctrl+W close tab, Ctrl+F find/replace, Ctrl+G go to line, Ctrl++/-
  zoom, Ctrl+Q quit.
- Atomic save (write `.tmp.note`, then rename).
- `maybeSave()` prompts before discarding.
- `applyTheme()` builds a stylesheet from theme colors + radius and updates
  all editors' current-line highlight.

### main.cpp

- `QApplication`, sets app name/display/version/desktop file name.
- `QCommandLineParser` with positional `file` arg and `--version`.

---

## 5. Build & Install Commands

```bash
cd /home/varo/Work/note

# Build
./bin/build          # runs qmake6 + make -j

# Install as Arch package
./bin/install        # runs makepkg -fsi (needs sudo/pkexec for install step)
# If makepkg can't elevate:
pkexec pacman -U --noconfirm /home/varo/Work/note/pkgbuild/note-0.1.0-1-x86_64.pkg.tar.zst

# Verify
note --version
desktop-file-validate /usr/share/applications/note.desktop
```

### note.pro

```qmake
QT += widgets
CONFIG += c++17
TARGET = note
TEMPLATE = app
SOURCES += src/main.cpp src/mainwindow.cpp src/editor.cpp \
           src/highlighter.cpp src/theme.cpp src/findreplace.cpp
HEADERS += src/mainwindow.h src/editor.h src/highlighter.h \
           src/theme.h src/findreplace.h
RESOURCES += src/resources.qrc
DESTDIR = build
```

### bin/build

```bash
#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p build
qmake6 note.pro
make -j"$(nproc)"
```

### bin/install

```bash
#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../pkgbuild"
makepkg -fsi --noconfirm
```

### pkgbuild/PKGBUILD (key parts)

```bash
pkgname=note
pkgver=0.1.0
pkgrel=1
arch=('x86_64' 'aarch64')
depends=('qt6-base' 'qt6-wayland')
makedepends=('base-devel' 'qt6-base')

build() {
    cd "$startdir/.."
    qmake6 note.pro
    make -j"$(nproc)"
}

package() {
    cd "$startdir/.."
    install -Dm755 build/note "$pkgdir/usr/bin/note"
    install -Dm644 "$startdir/note.desktop" "$pkgdir/usr/share/applications/note.desktop"
    install -Dm644 "$startdir/note.svg" "$pkgdir/usr/share/icons/hicolor/scalable/apps/note.svg"
}
```

---

## 6. Reproduction on a New Machine

1. Ensure Omarchy/Arch with Qt 6: `qmake6`, `make`, `g++`, `qt6-base`,
   `qt6-wayland`. Install missing with `omarchy-pkg-add base-devel qt6-base
   qt6-wayland` (or pacman).
2. Copy the `/home/varo/Work/note` directory to the new machine (e.g., via
   git or rsync). `git init` + commit if desired.
3. `cd note && ./bin/build`
4. `./bin/install` (elevate with pkexec if needed).
5. Launch from app launcher (`Super + Space` → Note) or run `note`.

### Optional: wire into Omarchy keybinds

Append to `~/.config/hypr/bindings.lua` (backup first):

```lua
o.bind("SUPER + SHIFT + E", "Text editor", { launch = "note" })
```

Then `hyprctl reload && hyprctl configerrors`.

---

## 7. Known Limitations (v1)

- UTF-8 only (no encoding picker).
- No print, plugins, preferences dialog, recent files.
- No full i18n (strings are English literals; Qt `tr()` not yet applied).
- Syntax highlighting is keyword-based (not a full language server).

## 8. Files Reference

All source is in `/home/varo/Work/note`. The installed artifacts:

- `/usr/bin/note`
- `/usr/share/applications/note.desktop`
- `/usr/share/icons/hicolor/scalable/apps/note.svg`
- Package: `/home/varo/Work/note/pkgbuild/note-0.1.0-1-x86_64.pkg.tar.zst`
