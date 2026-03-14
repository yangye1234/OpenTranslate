# OpenTranslate

[English](README.md) | 简体中文

一个基于 Qt Widgets 的轻量翻译工具，主打快速键盘操作、全局快捷键和本地 JSON 缓存离线命中。

![OpenTranslate Icon](assets/app-icon-1024.png)

## 功能特性

- 无边框悬浮 `Dialog` 主界面，支持拖动、置顶和快速翻译
- 独立 `Settings` 设置窗口，支持：
- 百度 API 配置（`AppID`、`AppKey`、`enabled`）
- 通用大模型 API 配置（`base_url`、`model`、`api_key`、`prompt`，V1 仅保存不调用）
- 语言方向管理（格式 `src->dst`，支持新增/编辑/删除）
- 应用语言切换：English / 简体中文 / 繁體中文
- 全局快捷键可配置并持久化
- 原文输入框按 `Enter` 仅触发翻译
- 结果自动聚焦并 `selectAll()`，方便复制
- JSON 缓存优先策略：
- 先查缓存，命中则不发在线请求
- 在线翻译成功后回写缓存
- 在线失败时自动尝试缓存兜底（离线复用）

## Provider（V1）

- `Baidu`：已接入可用
- `Generic API`：仅配置与保存，V1 不发请求

## 默认快捷键

- macOS：
- 语言互转：`Ctrl+Command+T`
- 置顶切换：`Ctrl+Command+F`
- 打开设置：`Ctrl+,`
- Windows：
- 语言互转：`Ctrl+Alt+T`
- 置顶切换：`Ctrl+Alt+F`
- 打开设置：`Ctrl+Alt+,`

快捷键可在设置页修改，保存后立即生效。

## 缓存说明

- 缓存文件：`QStandardPaths::AppDataLocation/translations_cache.json`
- 缓存结构：双向对象（单条支持 A<->B 双向命中）
- 匹配键：`provider + from + to + source_norm`
- `source_norm` 使用 `toCaseFolded()`，大小写不敏感
- 启动时会自动迁移旧版单向缓存格式

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

## 打包

参考 [PACKAGING.md](PACKAGING.md)，已覆盖：

- 从 `assets/app-icon-1024.png` 生成图标
- macOS `.app` / `.dmg`
- Windows `.exe` / NSIS 安装器

## 配置持久化

使用 `QSettings` 保存：

- 百度与通用 API 配置
- 语言方向列表
- 应用语言
- 全局快捷键

## 目录结构

```text
OpenTranslate/
├─ main.cpp
├─ translate.*                 # 主窗口与翻译流程
├─ settingswidget.*            # 设置页
├─ baidutranslatorservice.*    # 百度翻译实现
├─ translationcachestore.*     # 双向 JSON 缓存
├─ configstore.*               # QSettings 读写
├─ appconfig.h                 # 配置模型
├─ l10n.*                      # 多语言文案（en/zh-CN/zh-TW）
├─ scripts/generate_app_icons.sh
└─ PACKAGING.md
```

## 已知说明

- 全局快捷键在不同系统权限/输入法环境下可能受限制（尤其 macOS）
- 如 macOS 首次快捷键不生效，请为应用开启“辅助功能”权限
- 离线翻译能力基于缓存命中，不包含本地模型推理

## 路线图

- V1.1：接通 Generic API 实际翻译请求
- V1.2：缓存管理（清理/导入导出/开关）
- V1.3：更多 Provider（OpenAI/DeepL 等）

欢迎提 Issue 或 PR。
