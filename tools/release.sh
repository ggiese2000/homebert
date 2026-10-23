#!/usr/bin/env bash
set -euo pipefail

ROOT=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
BUILD_DIR="$ROOT/build"
RELEASE_DIR="$BUILD_DIR/release"
VERSION_FILE="$ROOT/VERSION"
BUILD_INFO="$BUILD_DIR/build-info.env"
RELEASE_INFO="$RELEASE_DIR/BUILD_INFO"
ARDUINO_DATA="$ROOT/.arduino/data/packages/esp32"
BOOT_APP0="$ARDUINO_DATA/hardware/esp32/3.3.8/tools/partitions/boot_app0.bin"
BOOTLOADER="$BUILD_DIR/firmware.ino.bootloader.bin"
PARTITIONS="$BUILD_DIR/firmware.ino.partitions.bin"
APPLICATION="$BUILD_DIR/firmware.ino.bin"
FULL="$RELEASE_DIR/homebert-full.bin"
UPDATE="$RELEASE_DIR/homebert-update.bin"

APP_OFFSET=0x10000
APP_SLOT_SIZE=$((0x1e0000))
FLASH_SIZE=$((4 * 1024 * 1024))

if [[ ! -f $VERSION_FILE ]]; then
  echo "missing authoritative version file: $VERSION_FILE" >&2
  exit 1
fi
VERSION=$(<"$VERSION_FILE")
if [[ ! $VERSION =~ ^(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)$ ]]; then
  echo "VERSION must be a Semantic Versioning release value" >&2
  exit 1
fi
if [[ -n $(git -C "$ROOT" status --porcelain --untracked-files=all) ]]; then
  echo "release generation requires a clean Git worktree" >&2
  exit 1
fi

"$ROOT/tools/build.sh"

ESPTOOL=$(find "$ARDUINO_DATA/tools/esptool_py" -type f -name esptool -perm -u+x -print -quit)
if [[ -z $ESPTOOL || ! -x $ESPTOOL ]]; then
  echo "esptool was not installed by the project-local Arduino-ESP32 setup" >&2
  exit 1
fi

if [[ ! -f $BUILD_INFO ]] || ! grep -Fxq "firmware_version=$VERSION" "$BUILD_INFO" ||
   ! grep -Fxq "git_commit=$(git -C "$ROOT" rev-parse HEAD)" "$BUILD_INFO"; then
  echo "build metadata does not match VERSION and HEAD; refusing stale artifacts" >&2
  exit 1
fi

for file in "$BOOT_APP0" "$BOOTLOADER" "$PARTITIONS" "$APPLICATION"; do
  if [[ ! -f $file ]]; then
    echo "required build image is missing: $file" >&2
    exit 1
  fi
done

app_size=$(stat -c '%s' "$APPLICATION")
if (( app_size > APP_SLOT_SIZE )); then
  echo "application image ($app_size bytes) exceeds OTA slot ($APP_SLOT_SIZE bytes)" >&2
  exit 1
fi

mkdir -p "$RELEASE_DIR"
rm -f "$FULL" "$UPDATE" "$RELEASE_DIR/SHA256SUMS" "$RELEASE_INFO"

# Match the Arduino-ESP32 3.3.8 ESP32 Dev Module upload recipe exactly.
# No NVS image is supplied. A full flash therefore leaves the NVS range erased,
# while the application-only update does not address that range.
"$ESPTOOL" --chip esp32 merge-bin --format raw \
  --flash-mode keep --flash-freq keep --flash-size keep \
  -o "$FULL" \
  0x1000 "$BOOTLOADER" \
  0x8000 "$PARTITIONS" \
  0xe000 "$BOOT_APP0" \
  "$APP_OFFSET" "$APPLICATION"

if ! cmp -n "$app_size" -i "0:$((APP_OFFSET))" "$APPLICATION" "$FULL"; then
  echo "merged image application bytes differ from the update image" >&2
  exit 1
fi

cp "$APPLICATION" "$UPDATE"

"$ESPTOOL" --chip esp32 image-info "$BOOTLOADER"
"$ESPTOOL" --chip esp32 image-info "$UPDATE"

if ! grep -aFq "$VERSION" "$UPDATE"; then
  echo "Release image does not contain firmware version $VERSION." >&2
  exit 1
fi

full_size=$(stat -c '%s' "$FULL")
if (( full_size > FLASH_SIZE )); then
  echo "merged image ($full_size bytes) exceeds 4 MB flash" >&2
  exit 1
fi

(
  cd "$RELEASE_DIR"
  sha256sum homebert-full.bin homebert-update.bin > SHA256SUMS
  sha256sum -c SHA256SUMS
)

{
  cat "$BUILD_INFO"
  printf 'application_offset=%s\n' "$APP_OFFSET"
  printf 'application_size=%s\n' "$app_size"
  printf 'application_slot_size=%s\n' "$APP_SLOT_SIZE"
  printf 'full_image_size=%s\n' "$full_size"
  printf 'full_image_sha256=%s\n' "$(sha256sum "$FULL" | awk '{print $1}')"
  printf 'update_image_sha256=%s\n' "$(sha256sum "$UPDATE" | awk '{print $1}')"
} > "$RELEASE_INFO"

printf 'Application: %s / %s bytes (%s bytes remaining)\n' \
  "$app_size" "$APP_SLOT_SIZE" "$((APP_SLOT_SIZE - app_size))"
printf 'Full image: %s / %s bytes\n' "$full_size" "$FLASH_SIZE"
printf 'First installation: esptool --chip esp32 write-flash 0x0 homebert-full.bin\n'
printf 'Application update: esptool --chip esp32 write-flash 0x10000 homebert-update.bin\n'
printf 'Build metadata: %s\n' "$RELEASE_INFO"
