#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SRC_PNG="${1:-$ROOT_DIR/assets/app-icon-1024.png}"
ICON_DIR="$ROOT_DIR/assets/icons"

if [[ ! -f "$SRC_PNG" ]]; then
  echo "Icon source not found: $SRC_PNG"
  echo "Please save your 1024x1024 app image to assets/app-icon-1024.png or pass a custom path."
  exit 1
fi

mkdir -p "$ICON_DIR"

if [[ "$OSTYPE" == darwin* ]]; then
  ICONSET_DIR="$ICON_DIR/app.iconset"
  rm -rf "$ICONSET_DIR"
  mkdir -p "$ICONSET_DIR"

  sips -z 16 16 "$SRC_PNG" --out "$ICONSET_DIR/icon_16x16.png" >/dev/null
  sips -z 32 32 "$SRC_PNG" --out "$ICONSET_DIR/icon_16x16@2x.png" >/dev/null
  sips -z 32 32 "$SRC_PNG" --out "$ICONSET_DIR/icon_32x32.png" >/dev/null
  sips -z 64 64 "$SRC_PNG" --out "$ICONSET_DIR/icon_32x32@2x.png" >/dev/null
  sips -z 128 128 "$SRC_PNG" --out "$ICONSET_DIR/icon_128x128.png" >/dev/null
  sips -z 256 256 "$SRC_PNG" --out "$ICONSET_DIR/icon_128x128@2x.png" >/dev/null
  sips -z 256 256 "$SRC_PNG" --out "$ICONSET_DIR/icon_256x256.png" >/dev/null
  sips -z 512 512 "$SRC_PNG" --out "$ICONSET_DIR/icon_256x256@2x.png" >/dev/null
  sips -z 512 512 "$SRC_PNG" --out "$ICONSET_DIR/icon_512x512.png" >/dev/null
  cp "$SRC_PNG" "$ICONSET_DIR/icon_512x512@2x.png"

  iconutil -c icns "$ICONSET_DIR" -o "$ICON_DIR/app.icns"
  rm -rf "$ICONSET_DIR"
  echo "Generated macOS icon: $ICON_DIR/app.icns"
fi

if command -v magick >/dev/null 2>&1; then
  magick "$SRC_PNG" -define icon:auto-resize=16,24,32,48,64,128,256 "$ICON_DIR/app.ico"
  echo "Generated Windows icon: $ICON_DIR/app.ico"
elif command -v convert >/dev/null 2>&1; then
  convert "$SRC_PNG" -define icon:auto-resize=16,24,32,48,64,128,256 "$ICON_DIR/app.ico"
  echo "Generated Windows icon: $ICON_DIR/app.ico"
else
  echo "ImageMagick not found. Install it to generate Windows .ico automatically."
fi

echo "Done."
