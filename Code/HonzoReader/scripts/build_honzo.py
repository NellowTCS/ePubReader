#!/usr/bin/env python3
"""Build honzo-c Rust crate into a C-compatible static library for ESP32-S3.

Usage:
    python3 scripts/build_honzo.py [debug|release]

Environment variables:
    OUTDIR        override output directory (default: lib/honzo_c/xtensa-esp32s3-espidf)
    HONZO_DIR     override path to honzo submodule (default: third_party/honzo)
    CARGO         override cargo command (default: cargo +esp)
"""

import os
import subprocess
import sys
import shutil


def main() -> None:
    script_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    honzo_dir = os.environ.get("HONZO_DIR", os.path.join(script_dir, "third_party", "honzo"))
    outdir = os.environ.get(
        "OUTDIR",
        os.path.join(script_dir, "lib", "honzo_c", "xtensa-esp32s3-espidf"),
    )
    cargo = os.environ.get("CARGO", "cargo +esp")

    profile = "release"
    if len(sys.argv) > 1:
        profile = sys.argv[1]

    if profile not in ("debug", "release"):
        print(f"Usage: {sys.argv[0]} [debug|release]", file=sys.stderr)
        sys.exit(1)

    target_subdir = profile
    cargo_flags = "--release" if profile == "release" else ""

    # Build honzo-c
    print(f"=== Building honzo-c ({profile}) ===")
    build_cmd = (
        f"{cargo} build {cargo_flags} "
        f"-p honzo-c --lib "
        f"--target xtensa-esp32s3-espidf "
        f"-Z build-std=std,panic_abort "
        f"--no-default-features"
    )
    subprocess.run(build_cmd, shell=True, cwd=honzo_dir, check=True)

    # Copy static library
    src = os.path.join(honzo_dir, "target", "xtensa-esp32s3-espidf", target_subdir, "libhonzo_c.a")
    dst_dir = os.path.join(outdir, target_subdir)
    os.makedirs(dst_dir, exist_ok=True)
    dst = os.path.join(dst_dir, "libhonzo_c.a")
    shutil.copy2(src, dst)

    st = os.stat(dst)
    print(f"=== {dst} ({st.st_size} bytes) ===")


if __name__ == "__main__":
    main()
