# OpenTranslate

English | [简体中文](README.zh-CN.md)

OpenTranslate is a lightweight cross-platform Qt Widgets translator for quick translation, selection translation, screenshot OCR, provider-based audio playback, cache, and history.

![OpenTranslate Icon](assets/app-icon2.png)

Current version: `0.2`

## Features

- Frameless floating translation window with drag, pin-to-top, compact controls, and fast keyboard flow.
- Language direction selector with `Auto` plus editable bidirectional pairs such as `zh <> en` and `ja <> zh`.
- Automatic source language detection and target language resolution from the selected language pair.
- Translation providers:
  - Baidu Translate
  - OpenAI-compatible Chat API
  - DeepL API
  - Youdao Dictionary, no API key required
- Provider audio playback when available, including Youdao dictionary audio and Baidu TTS URL support.
- Configurable global shortcuts:
  - Cycle language selection
  - Toggle always-on-top
  - Open settings
  - Translate selected text
  - Play / pause audio
  - Screenshot translate
- Selection translation using clipboard copy/restore flow.
- Screenshot translation:
  - macOS: Apple Vision OCR
  - Windows: Windows OCR with an MSVC Qt Kit and Windows SDK C++/WinRT headers, otherwise build-safe OCR stub
- Translation cache with enable/clear/import/export controls.
- Translation history with view/copy/clear controls and max-entry limit.
- Categorized settings UI for General, Services, Shortcuts, Privacy, and About.
- App language: English / 简体中文 / 繁體中文.

## Default Shortcuts

macOS:

- Cycle language selection: `Ctrl+Command+T`
- Toggle always-on-top: `Ctrl+Command+F`
- Open settings: `Ctrl+,`
- Translate selection: `Ctrl+Command+D`
- Play / pause audio: `Ctrl+Command+Space`
- Screenshot translate: `Ctrl+Command+A`

Windows:

- Cycle language selection: `Ctrl+Alt+T`
- Toggle always-on-top: `Ctrl+Alt+F`
- Open settings: `Ctrl+Alt+,`
- Translate selection: `Ctrl+Alt+D`
- Play / pause audio: `Ctrl+Alt+Space`
- Screenshot translate: `Ctrl+Alt+A`

Shortcuts can be edited in settings and take effect after saving.

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

macOS local debug build used during development:

```bash
cmake --build /Users/yy/code/qt/QtCpp/OpenTranslate/build/Qt_6_10_1_for_macOS-Debug --target all
```

## Packaging

See [PACKAGING.md](PACKAGING.md) for macOS and Windows packaging notes.

App icons are generated from `assets/app-icon2.png`:

```bash
./scripts/generate_app_icons.sh assets/app-icon2.png
```

The generator adds transparent padding for platform icons so Dock/taskbar sizing looks closer to native apps.

## Configuration And Data

Stored through `QSettings` and local JSON files:

- Provider settings and active provider
- Language pairs and default pair
- App language
- Global shortcuts
- Window behavior
- Cache and history preferences

Cache file:

- `QStandardPaths::AppDataLocation/translations_cache.json`

## Notes

- macOS selection translation requires Accessibility permission.
- macOS screenshot translation requires Screen Recording permission.
- Windows screenshot OCR requires an MSVC Qt Kit with Windows SDK C++/WinRT headers at build time. If `winrt/Windows.Foundation.h` is not found, OCR is disabled but the app still builds; install the Windows SDK C++/WinRT headers and reconfigure CMake to enable it.
- Offline behavior is cache-based only.
