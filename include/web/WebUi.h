#pragma once

#ifndef _WEBUI_h
#define _WEBUI_h

#include <cstddef>
#include <cstdint>
#include <ESPAsyncWebServer.h>

/// A file of the web UI build, embedded in the firmware (see ui/hexdump.py).
struct WebAsset {
    const char* path;
    const char* contentType;
    const uint8_t* data;
    size_t length;
    bool gzipped;
};

/// Serves the web UI's files, and the WebSocket it watches to show whether the board is reachable.
void addWebUiRoutes(AsyncWebServer& server);

#endif
