# OpenTranslate

English | [简体中文](README.zh-CN.md)

A lightweight Qt Widgets translator focused on quick keyboard flow, global shortcuts, and local JSON cache for offline hits.

![OpenTranslate Icon](assets/app-icon-1024.png)

## Features

- Frameless floating `Dialog` window with drag, pin-to-top, and fast translate flow
- Dedicated `Settings` window with:
- Baidu API config (`AppID`, `AppKey`, `enabled`)
- Generic LLM API config (`base_url`, `model`, `api_key`, `prompt`) saved only in V1
- Editable language pairs (`src->dst`)
- App language switch: English / 简体中文 / 繁體中文
- Configurable global shortcuts with persistence
- `Enter` in source input triggers translation only
- Result auto-focus + `selectAll()` for quick copy
- JSON cache-first strategy:
- Check cache first and skip API call on hit
- Write back cache after successful online translation
- If online fails, try cached fallback for offline reuse

## Providers (V1)

- `Baidu`: implemented and available
- `Generic API`: configuration only, not called in V1

## Default Shortcuts

- macOS:
- Swap language: `Ctrl+Command+T`
- Toggle on-top: `Ctrl+Command+F`
- Open settings: `Ctrl+,`
- Windows:
- Swap language: `Ctrl+Alt+T`
- Toggle on-top: `Ctrl+Alt+F`
- Open settings: `Ctrl+Alt+,`

All shortcuts can be edited in settings and take effect immediately after save.

## Cache

- File: `QStandardPaths::AppDataLocation/translations_cache.json`
- Format: bidirectional objects (single entry supports both A<->B lookups)
- Match key: `provider + from + to + source_norm`
- `source_norm` uses `toCaseFolded()` for case-insensitive matching
- Old one-way cache format is auto-migrated on startup

## Build

Requirements:

- Qt 6 (`Widgets`, `Network`, `LinguistTools`)
- CMake 3.16+
- C++17 compiler

Build:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
```

## Packaging

See [PACKAGING.md](PACKAGING.md) for:

- app icon generation from `assets/app-icon-1024.png`
- macOS `.app` / `.dmg`
- Windows `.exe` / NSIS installer

## Configuration Persistence

Stored via `QSettings`:

- Baidu and Generic API settings
- Language pairs
- App language
- Global shortcuts

## Project Structure

```text
OpenTranslate/
├─ main.cpp
├─ translate.*                 # main dialog and translate flow
├─ settingswidget.*            # settings window
├─ baidutranslatorservice.*    # baidu translator implementation
├─ translationcachestore.*     # bidirectional JSON cache
├─ configstore.*               # QSettings load/save
├─ appconfig.h                 # config models
├─ l10n.*                      # i18n (en/zh-CN/zh-TW)
├─ scripts/generate_app_icons.sh
└─ PACKAGING.md
```

## Notes

- Global shortcuts may be affected by system permissions/input-source environment (especially on macOS)
- If shortcuts do not trigger on first run in macOS, grant Accessibility permission to the app
- Offline behavior is cache-based only (no local model inference in V1)

## Roadmap

- V1.1: connect Generic API real requests
- V1.2: cache management (clean/import/export/switch)
- V1.3: more providers (OpenAI/DeepL/etc.)

Issues and PRs are welcome.
