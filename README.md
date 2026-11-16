# Omanote

A lightweight text editor for Omarchy.

![Omanote](docs/screenshot-omanote2.png)

## Features

- Tabbed editing
- Syntax highlighting for common languages (toggle)
- Line numbers and current-line highlight (toggle)
- Bracket matching
- Right margin
- Find and replace
- Word wrap, zoom
- Go to line
- Auto-indent (configurable)
- Configurable tab size and spaces/tabs
- Smart Home/End
- Restore cursor position
- Ensure trailing newline on save
- Backup copies
- Autosave
- Print support
- Preferences dialog
- Omarchy theme following (accent, rounding, dark mode)

## Quick Start

```bash
git clone https://github.com/KimiAlva/omanote.git
cd omanote
./bin/build
./build/omanote
```

## Build

```bash
./bin/build
```

## Run

```bash
./build/omanote
```

## Install (from source)

```bash
./bin/install
```

Or install the prebuilt package from the latest release:

Using GitHub CLI (if installed):
```bash
gh release download --repo KimiAlva/omanote --pattern 'omanote-*-x86_64.pkg.tar.zst'
pkexec pacman -U omanote-*-x86_64.pkg.tar.zst
```

Using curl + Python (no extra tools):
```bash
URL=$(curl -s https://api.github.com/repos/KimiAlva/omanote/releases/latest | python3 -c "import sys,json; print([a['browser_download_url'] for a in json.load(sys.stdin)['assets'] if a['name'].endswith('.pkg.tar.zst')][0])")
curl -LO "$URL"
pkexec pacman -U omanote-*-x86_64.pkg.tar.zst
```

## Dependencies

- qt6-base
- qt6-wayland
- syntax-highlighting (KSyntaxHighlighting)

## Shortcuts

| Key | Action |
|---|---|
| Ctrl+N | New tab |
| Ctrl+O | Open |
| Ctrl+S | Save |
| Ctrl+Shift+S | Save as |
| Ctrl+W | Close tab |
| Ctrl+F | Find/Replace |
| Ctrl+G | Go to line |
| Ctrl++ / Ctrl+- | Zoom in/out |
| Ctrl+Q | Quit |
| Tab | Insert 4 spaces |

## License

MIT
```<|end|><|start|>write to=assistant<|channel|>analysis<|message|>Created file successfully: note/README.md
