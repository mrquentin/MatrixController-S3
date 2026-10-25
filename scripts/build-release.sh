#!/usr/bin/env bash
# Builds the firmware for a release, called by semantic-release (.releaserc.json) with the
# version it picked, e.g. v1.2.0. Leaves the image in dist/firmware.bin for the GitHub release.
set -euo pipefail

version="$1"

# version.py writes this into the firmware, so the board reports the release's version
FIRMWARE_VERSION="$version" pio run -e adafruit_matrixportal_esp32s3

grep -q "\"$version\"" include/FirmwareVersion.h || {
    echo "Built firmware doesn't carry version $version:" >&2
    cat include/FirmwareVersion.h >&2
    exit 1
}

mkdir -p dist
cp .pio/build/adafruit_matrixportal_esp32s3/firmware.bin dist/firmware.bin
