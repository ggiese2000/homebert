#!/usr/bin/env bash
set -euo pipefail

ROOT=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
CONFIG="$ROOT/arduino-cli.yaml"
VERSION_FILE="$ROOT/VERSION"
BUILD_INFO="$ROOT/build/build-info.env"
FQBN='esp32:esp32:esp32:PartitionScheme=min_spiffs'
GENERATED_HEADER="$ROOT/firmware/homebert_build_info.h"

if [[ ! -f $VERSION_FILE ]]; then
  echo "missing authoritative version file: $VERSION_FILE" >&2
  exit 1
fi
VERSION=$(<"$VERSION_FILE")
if [[ ! $VERSION =~ ^(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)$ ]]; then
  echo "VERSION must be a Semantic Versioning release value (for example 0.3.5)" >&2
  exit 1
fi
if ! grep -Fq 'FirmwareRevision(kFirmwareVersion)' "$ROOT/firmware/firmware.ino"; then
  echo "firmware no longer exposes the injected version as FirmwareRevision" >&2
  exit 1
fi
GIT_COMMIT=$(git -C "$ROOT" rev-parse --verify HEAD)

trap 'rm -f "$GENERATED_HEADER"' EXIT
{
  printf '#pragma once\n'
  printf '#define HOMEBERT_BUILD_VERSION "%s"\n' "$VERSION"
  printf '#define HOMEBERT_BUILD_GIT_COMMIT "%s"\n' "$GIT_COMMIT"
} > "$GENERATED_HEADER"

if [[ -n ${ARDUINO_CLI:-} ]]; then
  CLI=$ARDUINO_CLI
elif [[ -x $ROOT/.tooling/arduino-cli/arduino-cli ]]; then
  CLI=$ROOT/.tooling/arduino-cli/arduino-cli
else
  CLI=arduino-cli
fi

# Arduino CLI uses its own proxy setting for package downloads.  In a managed
# environment, inherit the supplied HTTPS proxy without persisting it.
if [[ -z ${ARDUINO_NETWORK_PROXY:-} ]]; then
  export ARDUINO_NETWORK_PROXY=${HTTPS_PROXY:-${https_proxy:-}}
fi

if ! "$CLI" core update-index --config-file "$CONFIG"; then
  echo "Arduino package index refresh failed; using the installed pinned packages." >&2
fi
"$CLI" core install esp32:esp32@3.3.8 --config-file "$CONFIG"
"$CLI" lib install HomeSpan@2.1.8 --config-file "$CONFIG"
"$CLI" lib install NimBLE-Arduino@2.5.1 --config-file "$CONFIG"
"$CLI" compile --fqbn "$FQBN" \
  --warnings all \
  --output-dir "$ROOT/build" \
  --config-file "$CONFIG" \
  "$ROOT/firmware"

mkdir -p "$ROOT/build"
{
  printf 'firmware_version=%s\n' "$VERSION"
  printf 'git_commit=%s\n' "$GIT_COMMIT"
  printf 'fqbn=%s\n' "$FQBN"
  printf 'arduino_esp32=3.3.8\n'
  printf 'homespan=2.1.8\n'
  printf 'nimble_arduino=2.5.1\n'
  printf 'partition_scheme=min_spiffs\n'
} > "$BUILD_INFO"
