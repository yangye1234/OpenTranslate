# OpenTranslate 打包说明（macOS + Windows）

## 0. 先确认目录

所有命令都在**项目根目录**执行（不要在 `scripts/` 目录执行）：

```bash
cd /Users/yy/code/qt/QtCpp/OpenTranslate
```

## 1. 图标准备

1. 放置源图：
- `assets/app-icon-1024.png`
2. 生成系统图标：

```bash
./scripts/generate_app_icons.sh
```

生成结果：
- `assets/icons/app.icns`（macOS）
- `assets/icons/app.ico`（Windows）

## 2. macOS 打包

### 2.1 构建 `.app`

```bash
cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release
cmake --build build-release --config Release
```

产物：
- `build-release/OpenTranslate.app`

### 2.2 生成 `.dmg` 安装包

```bash
cd build-release
cpack -G DragNDrop
```

产物：
- `build-release/OpenTranslate-<version>-Darwin.dmg`

说明：
- 当前项目已在 CMake 安装阶段自动调用 Qt 部署脚本，因此通常**不需要手动执行** `macdeployqt`。

可选（仅当你想手工部署 `.app` 时）：

```bash
/Users/yy/Applications/qt/6.10.1/macos/bin/macdeployqt /Users/yy/code/qt/QtCpp/OpenTranslate/build-release/OpenTranslate.app
```

## 3. Windows 打包

建议在 Windows 机器上执行。

### 3.1 构建 `.exe`

```powershell
cmake -S . -B build-release -G "Ninja" -DCMAKE_BUILD_TYPE=Release
cmake --build build-release --config Release
```

### 3.2 生成安装器

NSIS（需 `makensis` 在 PATH）：

```powershell
cd build-release
cpack -G NSIS
```

ZIP 便携包：

```powershell
cpack -G ZIP
```

## 4. 常见问题

- 错误：`source directory ".../scripts" does not appear to contain CMakeLists.txt`
- 原因：在 `scripts/` 目录执行了 `cmake -S .`
- 解决：回到项目根目录执行

- 错误：`zsh: command not found: macdeployqt`
- 原因：Qt `bin` 未加入 PATH
- 解决：使用绝对路径调用（见上方可选命令）

- 错误：`Compatibility with CMake < 3.5 has been removed`
- 原因：CMake 4 + 依赖子项目旧策略
- 解决：本项目已内置兼容设置；如果仍遇到，可加：
  `-DCMAKE_POLICY_VERSION_MINIMUM=3.5`
