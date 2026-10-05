#!/usr/bin/env bash
# Build everything the way CI does: check hardware.yaml, build MCUboot + signed app
# (sysbuild), verify the built devicetree against hardware.lock.yaml, write SBOMs.
#
# Usage (from anywhere inside the west workspace):
#   zephyr-cra-starter/scripts/build.sh [build_dir] [extra west build args...]
# Release builds pass the signing key, e.g.:
#   scripts/build.sh build -- -DSB_CONFIG_BOOT_SIGNATURE_KEY_FILE=\"/secure/key.pem\"
#
# SPDX-License-Identifier: Apache-2.0
set -euo pipefail

REPO="$(cd "$(dirname "$0")/.." && pwd)"
WS="$(cd "$REPO/.." && pwd)"
BUILD="${1:-$WS/build}"
shift || true
case "$BUILD" in /*) ;; *) BUILD="$PWD/$BUILD" ;; esac

cd "$WS"
echo "== hwsync check"
python3 "$REPO/scripts/hwsync.py" check

echo "== build (sysbuild: MCUboot + signed application) -> $BUILD"
rm -rf "$BUILD"
# The SPDX generator needs CMake's file API query in place before configuring.
west spdx --init -d "$BUILD/app" >/dev/null
west spdx --init -d "$BUILD/mcuboot" >/dev/null
west build -b twin_sensor --sysbuild -d "$BUILD" "$REPO/app" "$@"

echo "== factory image (MCUboot + signed app)"
python3 "$WS/zephyr/scripts/build/mergehex.py" -o "$BUILD/merged.hex" \
  "$BUILD/mcuboot/zephyr/zephyr.hex" "$BUILD/app/zephyr/zephyr.signed.hex"

echo "== hwsync verify-build"
python3 "$REPO/scripts/hwsync.py" verify-build "$BUILD"

echo "== SBOM (SPDX 2.3)"
west spdx -d "$BUILD/app" --analyze-includes --include-sdk
west spdx -d "$BUILD/mcuboot"
mkdir -p "$BUILD/sbom/app" "$BUILD/sbom/mcuboot"
cp "$BUILD"/app/spdx/*.spdx "$BUILD/sbom/app/"
cp "$BUILD"/mcuboot/spdx/*.spdx "$BUILD/sbom/mcuboot/"
grep -h "^ExternalRef: SECURITY cpe23Type\|^ExternalRef: PACKAGE-MANAGER purl" "$BUILD"/sbom/*/*.spdx \
  | sort -u > "$BUILD/sbom/components.txt"
echo "SBOM written to $BUILD/sbom ($(wc -l < "$BUILD/sbom/components.txt") identified components)"
