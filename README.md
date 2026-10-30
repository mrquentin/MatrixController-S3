# Matrix Controller

Firmware and web UI for a HUB75 LED matrix driven by an [Adafruit MatrixPortal S3](https://www.adafruit.com/product/5778):
a clock, weather, live F1 timing (from MultiViewer) and scrolling text, set up from a phone.

- `src/`, `include/`: the firmware (PlatformIO, Arduino framework)
- `ui/`: the web UI (Svelte), built into the firmware as `include/ui/*.h`
- `android/`: a small Android app that shows the web UI full screen (see below)
- `test/`: firmware unit tests, run on the computer

## Building and flashing

```sh
npm ci --prefix ui            # once: the firmware build also builds the UI
pio run -t upload             # build and flash over USB
pio run -e ota -t upload      # or over WiFi, with MATRIX_UPDATE_PASSWORD set
```

On first start the panel shows a QR code for its setup network to pick a WiFi network. The web UI is then at
http://matrixcontroller.local. Firmware updates can be installed from its Settings → Firmware page,
either a `firmware.bin` or the latest GitHub release; they need the update password set on that page.
The controller looks for new releases by itself twice a day and flags them there.

The picture follows the board's accelerometer, so the panel can hang either way up (landscape only).
Settings → Panel → Orientation can fix it upright or flipped instead; Status → Orientation shows the
sensor's readings.

## Android app

Chrome only installs a real app for HTTPS sites and the board serves plain HTTP, so `android/` wraps
the web UI in a full-screen WebView instead (about 30 KB, no libraries). Every release has it as
`MatrixController.apk`; the latest one is always at
https://github.com/mrquentin/MatrixController-S3/releases/latest/download/MatrixController.apk.
Installing a newer one over it keeps its settings. To build it yourself:

```sh
cd android
./gradlew assembleRelease     # app/build/outputs/apk/release/app-release.apk, signed with the debug key
adb install -r app/build/outputs/apk/release/app-release.apk
```

It opens http://matrixcontroller.local (the board's mDNS name). If nothing answers there it offers to
retry; long-press the app icon → Change address to point it somewhere else.

## Tests

```sh
pio test -e native            # firmware unit tests (needs a host C++ compiler)
npm test --prefix ui          # UI tests
npm run check --prefix ui     # UI type checks
```

## Releases

Every pull request runs the UI checks, the firmware tests and the firmware build
(`.github/workflows/ci.yml`). Merging to `main` releases automatically
(`.github/workflows/release.yml`): [semantic-release](https://semantic-release.gitbook.io/) picks the
next version from the commits since the last release, builds the firmware with it and publishes a
GitHub release with `firmware.bin` and the Android app (`MatrixController.apk`, same version). Nobody
edits version numbers.

Release APKs are signed with a fixed key so each installs over the last, kept in the repository's
Actions secrets: `ANDROID_KEYSTORE_BASE64` (the keystore, base64), `ANDROID_KEYSTORE_PASSWORD`,
`ANDROID_KEY_ALIAS` and `ANDROID_KEY_PASSWORD`. Losing it means uninstalling the app once to move to
a new key.

Pull requests are squash-merged, so their title becomes the commit message and must follow
[Conventional Commits](https://www.conventionalcommits.org/):

| Title                                  | Release                |
|----------------------------------------|------------------------|
| `fix: clock skips a second`            | patch, 1.2.0 → 1.2.1   |
| `feat: add a countdown app`            | minor, 1.2.0 → 1.3.0   |
| `feat!: drop 32 px tall panels`        | major, 1.2.0 → 2.0.0   |
| `docs:`, `chore:`, `ci:`, `test:`, `refactor:` | none (ships with the next release) |
