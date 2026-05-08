#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DEFAULT_SRC_PNG="$ROOT_DIR/assets/app-icon2.png"
if [[ ! -f "$DEFAULT_SRC_PNG" ]]; then
  DEFAULT_SRC_PNG="$ROOT_DIR/assets/app-icon-1024.png"
fi
SRC_PNG="${1:-$DEFAULT_SRC_PNG}"
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
  sips -z 1024 1024 "$SRC_PNG" --out "$ICONSET_DIR/icon_512x512@2x.png" >/dev/null

  xattr -cr "$ICONSET_DIR" 2>/dev/null || true
  if iconutil -c icns "$ICONSET_DIR" -o "$ICON_DIR/app.icns"; then
    :
  elif command -v node >/dev/null 2>&1; then
    node - "$ICONSET_DIR" "$ICON_DIR/app.icns" <<'NODE'
const fs = require('fs');
const path = require('path');

const [iconsetDir, outputFile] = process.argv.slice(2);
const items = [
  ['icp4', 'icon_16x16.png'],
  ['icp5', 'icon_32x32.png'],
  ['icp6', 'icon_32x32@2x.png'],
  ['ic07', 'icon_128x128.png'],
  ['ic08', 'icon_256x256.png'],
  ['ic09', 'icon_512x512.png'],
  ['ic10', 'icon_512x512@2x.png'],
];

const chunks = items.map(([type, fileName]) => {
  const data = fs.readFileSync(path.join(iconsetDir, fileName));
  const header = Buffer.alloc(8);
  header.write(type, 0, 4, 'ascii');
  header.writeUInt32BE(data.length + 8, 4);
  return Buffer.concat([header, data]);
});

const totalLength = 8 + chunks.reduce((sum, chunk) => sum + chunk.length, 0);
const header = Buffer.alloc(8);
header.write('icns', 0, 4, 'ascii');
header.writeUInt32BE(totalLength, 4);
fs.writeFileSync(outputFile, Buffer.concat([header, ...chunks]));
NODE
  else
    echo "iconutil failed and Node.js is not available to create macOS .icns fallback."
    exit 1
  fi
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
