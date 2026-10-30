#!/usr/bin/env bash
# Builds the firmware and the Android app for a release, called by semantic-release (.releaserc.json)
# with the version it picked, e.g. v1.2.0. Leaves dist/firmware.bin and dist/MatrixController.apk for
# the GitHub release.
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

# The app gets the same version, without the v. Signed with the release key when the workflow
# provides one (ANDROID_KEYSTORE_FILE), otherwise with a throwaway debug key that can't update
# an app installed from another release.
if [ -z "${ANDROID_KEYSTORE_FILE:-}" ]; then
    echo "::warning::No Android release key, the APK is signed with a debug key"
fi
(cd android && ./gradlew --no-daemon assembleRelease -PappVersion="${version#v}")
cp android/app/build/outputs/apk/release/app-release.apk dist/MatrixController.apk
