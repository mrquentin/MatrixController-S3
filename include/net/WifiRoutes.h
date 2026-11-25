#pragma once

#ifndef _WIFIROUTES_h
#define _WIFIROUTES_h

#include <ESPAsyncWebServer.h>
#include "SystemManager.h"

/// GET /api/system/wifi/scan: the networks in range, see WifiScan::listNetworks. Also served by the
/// setup portal.
void addWifiScanRoute(AsyncWebServer& server);

/// The web UI's WiFi settings: the saved network, joining another one, forgetting it.
/// Joining or forgetting restarts the board.
void addWifiRoutes(AsyncWebServer& server, SystemManager& system);

#endif
