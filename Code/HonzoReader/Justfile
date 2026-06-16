env := "OTA_APP"
profile := "release"

# Build honzo-c static library for ESP32
build-honzo:
    python3 scripts/build_honzo.py {{profile}}

# Build PlatformIO firmware
build-firmware:
    python3 scripts/build_firmware.py {{env}}

# Package firmware.bin into HonzoReader.tar
package:
    python3 scripts/package_ota.py {{env}}

# Full build pipeline: honzo-c -> firmware -> OTA tarball
build: build-honzo build-firmware package

# Clean build artifacts
clean:
    rm -rf .pio
    rm -rf lib/honzo_c
