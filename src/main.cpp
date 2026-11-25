#include <Arduino.h>
#include <cstdio>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include "Config.h"
#include "Log.h"
#include "SystemClock.h"
#include "SystemManager.h"
#include "SystemRoutes.h"
#include "apps/AppManager.h"
#include "apps/AppRoutes.h"
#include "apps/ClockApp.h"
#include "apps/F1App.h"
#include "apps/TextApp.h"
#include "apps/WeatherApp.h"
#include "display/Hub75Display.h"
#include "display/TestPattern.h"
#include "display/Text.h"
#include "net/Hostname.h"
#include "net/WifiRoutes.h"
#include "net/WifiSetup.h"
#include "net/WifiStore.h"
#include "ota/FirmwareUpdate.h"
#include "ota/NetworkUpload.h"
#include "ota/ReleaseUpdate.h"
#include "ota/UpdateRoutes.h"
#include "ota/UpdateScreen.h"
#include "sensors/AutoOrientation.h"
#include "web/Api.h"
#include "web/WebUi.h"

AsyncWebServer server(80);

Config config("/config.json");
SystemManager systemManager;
AppManager appManager("/apps.json");
FirmwareUpdate firmwareUpdate;
NetworkUpload networkUpload(firmwareUpdate);
ReleaseUpdate releaseUpdate(firmwareUpdate);
UpdateScreen updateScreen(firmwareUpdate);
AutoOrientation autoOrientation;
Hub75Display display;
WifiSetup wifiSetup(server, systemManager);

TextApp textApp;
ClockApp clockApp;
WeatherApp weatherApp;
F1App f1App;

// Boot: test pattern, connecting screen, then a screen with the IP address before the apps start
constexpr uint32_t TestPatternDurationMs = 3000;
constexpr uint32_t AddressScreenDurationMs = 4000;
uint32_t setupDoneMs = 0;
constexpr uint32_t FrameIntervalMs = 1000 / 60;

bool setupMode = false;
// The config revision the panel and clock are set up with
uint32_t appliedConfig = 0;

namespace {
    // Shown at boot so the address can be read off the panel without a serial cable
    void drawAddressScreen() {
        char ip[16];
        const IPAddress addr = WiFi.localIP();
        snprintf(ip, sizeof(ip), "%u.%u.%u.%u", addr[0], addr[1], addr[2], addr[3]);
        showStatus(display, "Matrix Controller", ip);
    }

    void startNormalMode(const SystemSettings& settings) {
        SystemClock::begin(settings.timezone);

        appManager.add(clockApp);
        appManager.add(weatherApp);
        appManager.add(f1App);
        appManager.add(textApp);
        appManager.begin();

        MDNS.begin(Hostname);

        // Uploads from PlatformIO block loop(), so they draw their own progress
        networkUpload.onProgress([] { updateScreen.drawProgress(display); });
        networkUpload.begin();

        allowCrossOrigin(server);
        addWebUiRoutes(server);
        addSystemRoutes(server, systemManager, config, autoOrientation);
        addAppRoutes(server, appManager);
        addWifiRoutes(server, systemManager);
        addUpdateRoutes(server, firmwareUpdate, networkUpload, releaseUpdate);
        server.begin();
    }

    /// Settings saved from the web UI, applied between frames: only the loop task may draw
    void applySavedConfig() {
        if (config.revision() == appliedConfig) return;
        appliedConfig = config.revision();
        const SystemSettings settings = config.get();
        Log::setEnabled(settings.debug);
        display.setBrightness(settings.display.brightness);
        autoOrientation.setMode(settings.display.orientation);
        SystemClock::setTimezone(settings.timezone);
    }
}

void setup() {
    Serial.begin(115200);
    LOGF("Booted\n");

    // The Matrix Portal's partition table (partitions-8MB-tinyuf2.csv) has no "spiffs" partition,
    // which is LittleFS's default label. Its data partition is "ffat", where uploadfs writes the image.
    if (!LittleFS.begin(false, "/littlefs", 10, "ffat")) LOGF("Failed to mount LittleFS\n");

    config.load();
    const SystemSettings settings = config.get();
    Log::setEnabled(settings.debug);

    const WifiCredentials credentials = WifiStore::load();
    setupMode = wifiSetup.setupModeDue(credentials);

    // The panel comes first so every WiFi step after this can be shown on it
    DisplayConfig displayConfig = settings.display;
    if (setupMode) WifiSetup::adjustForSetupMode(displayConfig);
    // Before anything is drawn, so even the test pattern is the right way up
    autoOrientation.begin(displayConfig.orientation);
    display.setOrientation(autoOrientation.current());
    if (display.begin(displayConfig)) {
        drawTestPattern(display);
        display.present();
        delay(TestPatternDurationMs);
    }

    if (setupMode) {
        wifiSetup.beginSetupMode(display);
        server.begin();
    } else if (wifiSetup.join(display, credentials)) {
        startNormalMode(settings);
    } else {
        wifiSetup.restartIntoSetupMode(display, credentials.ssid);
    }
    setupDoneMs = millis();
}

void loop() {
    const uint32_t now = millis();
    // Ends the probation of freshly updated firmware once it has been running fine for a while;
    // a crash or restart before that rolls back to the previous one
    FirmwareUpdate::confirmWhenHealthy(now - setupDoneMs);

    // Only between frames: every screen redraws in full, so the next one comes out the new way up
    display.setOrientation(autoOrientation.update(now));

    // An installed update boots after a restart. Restarting also remounts LittleFS after a file
    // system update. Waiting a moment lets the web UI get its answer first
    if (firmwareUpdate.state() == FirmwareUpdate::State::Installed) {
        systemManager.scheduleRestart(firmwareUpdate.replacesFiles() ? "Files updated" : "Update installed");
    }

    // Asked for from the web UI or the setup portal: say so on the panel until it happens
    if (systemManager.restartPending()) {
        static bool shown = false;
        if (!shown) {
            shown = true;
            showStatus(display, "Restarting...", systemManager.restartNote());
        }
        systemManager.restartIfDue(now);
        delay(5);
        return;
    }

    if (setupMode) {
        wifiSetup.loopSetupMode(display, now);
        delay(5);
        return;
    }

    networkUpload.handle();
    // Looks for new GitHub releases in the background now and then, for the web UI to offer
    releaseUpdate.checkWhenDue(now);

    if (updateScreen.draw(display, now)) {
        delay(5);
        return;
    }

    applySavedConfig();

    static uint32_t lastFrameMs = 0;
    if (now - lastFrameMs < FrameIntervalMs) {
        delay(1);
        return;
    }
    lastFrameMs = now;

    if (now - setupDoneMs < AddressScreenDurationMs) {
        drawAddressScreen();
    } else {
        appManager.render(display, now);
    }
}
