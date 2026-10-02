#pragma once

#include <ESPAsyncWebServer.h>
#include "SystemManager.h"

#ifndef _WSHANDLER_h
#define _WSHANDLER_h

class WebSocketHandler {
public:
    WebSocketHandler(AsyncWebServer& server, AsyncWebSocket& webSocket, SystemManager& systemManager);
    void setup();
private:
    AsyncWebServer& _server;
    AsyncWebSocket& _webSocket;
    SystemManager& _systemManager;
};

#endif
