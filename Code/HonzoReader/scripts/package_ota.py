#!/usr/bin/env python3
"""Package firmware.bin into a tarball for OTA upload.

Usage:
    python3 scripts/package_ota.py [env]

Output: HonzoReader.tar in the project root.
"""

import os
import subprocess
import sys
import tempfile


def main() -> None:
    pio_env = sys.argv[1] if len(sys.argv) > 1 else os.environ.get("ENV", "OTA_APP")
    script_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

    bin_path = os.path.join(script_dir, ".pio", "build", pio_env, "firmware.bin")
    tar_path = os.path.join(script_dir, "HonzoReader.tar")

    if not os.path.isfile(bin_path):
        print(f"Error: {bin_path} not found", file=sys.stderr)
        sys.exit(1)

    with tempfile.TemporaryDirectory() as tmp:
        tmpbin = os.path.join(tmp, "firmware.bin")
        import shutil
        shutil.copy2(bin_path, tmpbin)
        subprocess.run(
            ["tar", "cf", tar_path, "-C", tmp, "firmware.bin"],
            check=True,
        )

    st = os.stat(tar_path)
    print(f"=== {tar_path} ({st.st_size} bytes) ===")


if __name__ == "__main__":
    main()
