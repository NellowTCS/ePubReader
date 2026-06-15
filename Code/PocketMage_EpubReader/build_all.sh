#!/bin/bash
# Full firmware build pipeline:
#   1. Build Rust lexepub -> liblexepub.a
#   2. Build PlatformIO firmware (links against liblexepub.a)
#   3. Package firmware.bin into a tarball for OTA upload
#
# Usage:
#   ./build_all.sh                          # release build, OTA_APP env
#   ./build_all.sh debug                    # debug build, OTA_APP env
#   ENV=ota_ota ./build_all.sh              # custom PlatformIO env
#   SKIP_LEXEPUB=1 ./build_all.sh           # skip Rust build (reuse cached)
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

PIO_ENV="${ENV:-OTA_APP}"
LEXEPUB_PROFILE="${1:-release}"

# Rust static library
if [ -z "${SKIP_LEXEPUB:-}" ]; then
    echo ""
    echo "1/3  Build lexepub ($LEXEPUB_PROFILE)"
    "$SCRIPT_DIR/build_lexepub.sh" "$LEXEPUB_PROFILE"
else
    echo "1/3  SKIP_LEXEPUB set, reusing existing liblexepub.a"
fi

# PlatformIO firmware
echo ""
echo "2/3  PlatformIO ($PIO_ENV)"
pio run -e "$PIO_ENV"

# Package tarball for OTA
echo ""
echo "3/3  Package firmware.bin -> EpubReader.tar"
BIN="$SCRIPT_DIR/.pio/build/$PIO_ENV/firmware.bin"
TAR="$SCRIPT_DIR/EpubReader.tar"

if [ -f "$BIN" ]; then
    tmpbin="$(mktemp /tmp/EpubReader.XXXXXX.bin)"
    cp "$BIN" "$tmpbin"
    tar cf "$TAR" -C "$(dirname "$tmpbin")" "$(basename "$tmpbin")"
    rm -f "$tmpbin"
    ls -lh "$TAR"
    echo ""
    echo "Done — $TAR"
else
    echo "Error: $BIN not found" >&2
    echo "  Did PlatformIO build succeed? Check the output above." >&2
    exit 1
fi
