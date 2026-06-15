#!/bin/bash
# Build lexepub Rust crate into a C-compatible static library for the ESP32-S3.
#
# Prerequisites:
#   - espup installed and the Xtensa ESP target toolchain sourced
#     (run: source "$HOME/export-esp.sh")
#   - Rust with the esp toolchain (rustup toolchain install esp)
#
# Usage:
#   ./build_lexepub.sh                        # release build
#   ./build_lexepub.sh debug                  # debug build
#   OUTDIR=/custom/path ./build_lexepub.sh    # override output dir
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
: "${LEXEPUB_DIR:="$SCRIPT_DIR/third_party/lexepub/lexepub"}"
: "${OUTDIR:="$SCRIPT_DIR/lib/lexepub_c/xtensa-esp32s3-espidf"}"

# Detect and activate toolchains
check_prereqs() {
    if ! command -v cargo &>/dev/null; then
        for try in "$HOME/.cargo/env" "$HOME/.rustup/env"; do
            [ -f "$try" ] && { . "$try"; break; }
        done
    fi
    if ! command -v cargo &>/dev/null; then
        echo "Error: cargo not found.  Install Rust at https://rustup.rs" >&2
        exit 1
    fi

    if ! command -v xtensa-esp32s3-elf-gcc &>/dev/null; then
        for try in "$HOME/export-esp.sh" "$HOME/esp/esp-idf/export.sh" \
                   /opt/esp/export.sh /opt/esp-idf/export.sh; do
            if [ -f "$try" ]; then
                echo "   Sourcing $try ..."
                . "$try"
                break
            fi
        done
    fi
    if ! command -v xtensa-esp32s3-elf-gcc &>/dev/null; then
        echo "Error: xtensa-esp32s3-elf-gcc not found.  Source your ESP-IDF env first:" >&2
        echo "    source \$HOME/export-esp.sh" >&2
        exit 1
    fi
}

check_prereqs

# Cargo with ESP toolchain
CARGO="cargo"
if cargo +esp version &>/dev/null 2>&1; then
    CARGO="cargo +esp"
fi

# Build
PROFILE="${1:-release}"
case "$PROFILE" in
    debug)   CARGO_FLAGS="";          TARGET_SUBDIR="debug"   ;;
    release) CARGO_FLAGS="--release"; TARGET_SUBDIR="release" ;;
    *)       echo "Usage: $0 [debug|release]" >&2; exit 1
esac

echo "=== Building lexepub ($PROFILE) ==="
cd "$LEXEPUB_DIR"
$CARGO build $CARGO_FLAGS \
    --features "c-ffi,lowmem" -p lexepub --lib \
    --target xtensa-esp32s3-espidf \
    -Z build-std=std,panic_abort

# Copy the static library
# Cargo places the target at workspace root, not crate dir.
WORKSPACE_ROOT="$(dirname "$LEXEPUB_DIR")"
SRC="$WORKSPACE_ROOT/target/xtensa-esp32s3-espidf/$TARGET_SUBDIR/liblexepub.a"
OUT="$OUTDIR/$TARGET_SUBDIR/liblexepub.a"

mkdir -p "$OUTDIR/$TARGET_SUBDIR"
cp "$SRC" "$OUT"

echo "=== $OUT ==="
ls -lh "$OUT"
