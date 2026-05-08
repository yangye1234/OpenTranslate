# OpenTranslate

[English](README.md) | 简体中文

OpenTranslate 是一个基于 Qt Widgets 的轻量全平台翻译工具，支持快速翻译、选中文本翻译、截图 OCR、翻译服务语音播放、缓存和历史记录。

![OpenTranslate Icon](assets/app-icon2.png)

当前版本：`0.2`

## 功能特性

- 无边框悬浮翻译窗口，支持拖动、置顶、紧凑控制和快捷键操作。
- 语言方向下拉框支持 `自动选择` 和可编辑双向语言对，例如 `zh <> en`、`ja <> zh`。
- 自动识别源语言，并根据当前语言对决定目标语言。
- 翻译服务：
  - 百度翻译
  - OpenAI 兼容 Chat API
  - DeepL API
  - 有道词典，无需 API Key
- 支持翻译服务语音播放，包括有道词典音频和百度 TTS URL。
- 全局快捷键可配置：
  - 按列表顺序切换语言方向
  - 切换置顶
  - 打开设置
  - 翻译选中文本
  - 播放 / 暂停语音
  - 截图翻译
- 选中文本翻译使用剪贴板复制/恢复策略。
- 截图翻译：
  - macOS：Apple Vision OCR
  - Windows：构建环境有 C++/WinRT 头文件时启用 Windows OCR，否则降级为可编译 stub
- 翻译缓存支持启用、清理、导入、导出。
- 翻译历史支持查看、复制、清空和最大条数限制。
- 设置页按通用、服务、快捷键、隐私、关于分类。
- 应用语言：English / 简体中文 / 繁體中文。

## 默认快捷键

macOS：

- 按顺序切换语言方向：`Ctrl+Command+T`
- 切换置顶：`Ctrl+Command+F`
- 打开设置：`Ctrl+,`
- 翻译选中文本：`Ctrl+Command+D`
- 播放 / 暂停语音：`Ctrl+Command+Space`
- 截图翻译：`Ctrl+Command+A`

Windows：

- 按顺序切换语言方向：`Ctrl+Alt+T`
- 切换置顶：`Ctrl+Alt+F`
- 打开设置：`Ctrl+Alt+,`
- 翻译选中文本：`Ctrl+Alt+D`
- 播放 / 暂停语音：`Ctrl+Alt+Space`
- 截图翻译：`Ctrl+Alt+A`

快捷键可在设置页修改，保存后生效。

## 构建

依赖：

- Qt 6（`Widgets`, `Network`, `LinguistTools`）
- CMake 3.16+
- C++17 编译器

构建命令：

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
```

开发时使用的 macOS Debug 构建命令：

```bash
cmake --build /Users/yy/code/qt/QtCpp/OpenTranslate/build/Qt_6_10_1_for_macOS-Debug --target all
```

## 打包

macOS 和 Windows 打包说明见 [PACKAGING.md](PACKAGING.md)。

应用图标从 `assets/app-icon2.png` 生成：

```bash
./scripts/generate_app_icons.sh assets/app-icon2.png
```

生成器会为平台图标添加透明边距，让 Dock/任务栏里的视觉尺寸更接近原生应用。

## 配置与数据

通过 `QSettings` 和本地 JSON 保存：

- 翻译服务配置和当前服务
- 语言方向列表和默认语言对
- 应用语言
- 全局快捷键
- 窗口行为
- 缓存和历史设置

缓存文件：

- `QStandardPaths::AppDataLocation/translations_cache.json`

## 已知说明

- macOS 选中文本翻译需要授予“辅助功能”权限。
- macOS 截图翻译需要授予“屏幕录制”权限。
- Windows 截图 OCR 构建时需要 Windows SDK C++/WinRT 头文件；没有时 OCR 会禁用，但主程序仍可构建。
- 离线能力基于缓存命中，不包含本地模型推理。
