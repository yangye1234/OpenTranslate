# OpenTranslate 打包说明（macOS + Windows）

## 1. 准备应用图标

1. 把你发的这张图保存为：
   - `assets/app-icon-1024.png`
2. 生成系统图标：

```bash
./scripts/generate_app_icons.sh
```

生成结果：
- `assets/icons/app.icns`（macOS）
- `assets/icons/app.ico`（Windows）

## 2. macOS 打包

建议在 macOS 机器上执行。

### 2.1 生成 `.app` 可执行包

```bash
cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release
cmake --build build-release --config Release
```

产物：
- `build-release/OpenTranslate.app`

### 2.2 拷贝 Qt 依赖并可直接运行

```bash
macdeployqt build-release/OpenTranslate.app
```

### 2.3 生成 `.dmg` 安装包

```bash
cmake --install build-release --config Release --prefix build-release/install
cd build-release
cpack -G DragNDrop
```

产物：
- `build-release/OpenTranslate-<version>-Darwin.dmg`

## 3. Windows 打包

建议在 Windows 机器上执行（MSVC 或 MinGW 环境均可）。

### 3.1 生成 `.exe`

```powershell
cmake -S . -B build-release -G "Ninja" -DCMAKE_BUILD_TYPE=Release
cmake --build build-release --config Release
```

产物：
- `build-release/OpenTranslate.exe`

### 3.2 拷贝 Qt 运行库

```powershell
windeployqt build-release\OpenTranslate.exe
```

### 3.3 生成安装器（NSIS）

确保已安装 NSIS，并且 `makensis` 在 PATH 中：

```powershell
cmake --install build-release --config Release --prefix build-release\install
cd build-release
cpack -G NSIS
```

产物：
- `build-release/OpenTranslate-<version>-Windows.exe`（安装器）

可选 ZIP 便携包：

```powershell
cpack -G ZIP
```

## 4. 注意事项

- 必须在对应系统上打对应包：mac 上打 `.app/.dmg`，Windows 上打 `.exe/.nsis`。
- 如果提示缺少图标，请确认已生成：
  - `assets/icons/app.icns`
  - `assets/icons/app.ico`
- 若首次运行提示快捷键权限问题，在 macOS 里到“系统设置 -> 隐私与安全性 -> 辅助功能”给应用授权。
