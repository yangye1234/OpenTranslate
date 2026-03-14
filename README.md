# OpenTranslate

一个基于 Qt Widgets 的轻量翻译工具，主打快速输入、快捷键操作和本地缓存离线命中。

![OpenTranslate Icon](assets/app-icon-1024.png)

## 功能特性

- 无边框悬浮 `Dialog` 主界面，支持拖动、置顶和快速翻译
- `Settings` 设置页（独立窗口）：
- 百度翻译 API 配置（`AppID/AppKey/启用`）
- 通用大模型 API 配置（V1 仅保存配置，不发请求）
- 语言方向管理（新增/编辑/删除，格式 `src->dst`）
- 应用语言切换：English / 简体中文 / 繁體中文
- 全局快捷键（可配置并持久化）：
- 语言互转
- 窗口置顶切换
- 打开设置
- 回车翻译链路：
- 在原文输入框按 `Enter` 触发翻译
- 成功后结果自动填充并聚焦到结果框，方便复制
- JSON 翻译缓存（大小写不敏感，双向命中）：
- 优先本地缓存命中，命中则不发网络请求
- 未命中调用在线翻译，成功后写回缓存
- 在线失败时自动尝试缓存兜底（离线命中）

## 当前翻译 Provider（V1）

- `Baidu`：已接入可用
- `Generic API`：仅保存 `base_url/model/api_key/prompt` 配置，V1 不调用

## 快捷键默认值

- macOS
- 语言互转：`Ctrl+Command+T`
- 置顶切换：`Ctrl+Command+F`
- 打开设置：`Ctrl+,`
- Windows
- 语言互转：`Ctrl+Alt+T`
- 置顶切换：`Ctrl+Alt+F`
- 打开设置：`Ctrl+Alt+,`

说明：快捷键可在设置页修改，保存后立即生效并写入 `QSettings`。

## 缓存说明

- 文件路径：`QStandardPaths::AppDataLocation/translations_cache.json`
- 结构：双向对象（单条同时包含 A<->B 两侧文本）
- 命中键：`provider + from + to + source_norm`
- `source_norm` 使用 `toCaseFolded()`，大小写不敏感
- 启动时会自动迁移旧版单向缓存格式为双向格式

## 构建（开发）

### 依赖

- Qt 6（`Widgets`, `Network`, `LinguistTools`）
- CMake 3.16+
- C++17 编译器

### 本地构建

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
```

## 打包发布

已提供完整打包文档：

- [PACKAGING.md](PACKAGING.md)

包含：
- 使用 `assets/app-icon-1024.png` 生成 `icns/ico`
- macOS 打包 `.app` / `.dmg`
- Windows 打包 `.exe` / NSIS 安装器

## 配置持久化

使用 `QSettings` 存储：

- 百度与通用 API 配置
- 语言方向列表
- 应用语言
- 全局快捷键

## 项目结构

```text
OpenTranslate/
├─ main.cpp
├─ translate.*                 # 主窗口与翻译流程
├─ settingswidget.*            # 设置页
├─ baidutranslatorservice.*    # 百度翻译实现
├─ translationcachestore.*     # JSON 双向缓存
├─ configstore.*               # QSettings 读写
├─ appconfig.h                 # 配置模型
├─ l10n.*                      # 中/英/繁本地化
├─ scripts/generate_app_icons.sh
└─ PACKAGING.md
```

## 已知说明

- 全局快捷键在不同系统输入法/权限环境下可能受限制（尤其 macOS）
- 若 macOS 首次无法触发全局快捷键，请检查系统“辅助功能”权限
- 当前离线能力基于缓存命中，不包含本地模型推理

## 版本路线（建议）

- V1.1：接通 Generic API 实际请求
- V1.2：缓存管理（清理、导入导出、禁用缓存开关）
- V1.3：更多翻译 Provider（OpenAI/DeepL 等）

---

如果你在使用中遇到问题，欢迎提 Issue 或直接提交 PR。
