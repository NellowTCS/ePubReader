#!/usr/bin/env python3
"""Build PlatformIO firmware.

Usage:
    python3 scripts/build_firmware.py [env]

Defaults to OTA_APP if no env is given.
"""

import os
import subprocess
import sys


def main() -> None:
    pio_env = sys.argv[1] if len(sys.argv) > 1 else os.environ.get("ENV", "OTA_APP")
    script_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

    print(f"=== PlatformIO ({pio_env}) ===")
    subprocess.run(
        ["pio", "run", "-e", pio_env],
        cwd=script_dir,
        check=True,
    )


if __name__ == "__main__":
    main()
