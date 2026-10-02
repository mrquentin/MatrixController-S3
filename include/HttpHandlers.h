#pragma once

#include <ESPAsyncWebServer.h>

#include "Config.h"
#include "SystemManager.h"

#ifndef _HTTPHANDLERS_h
#define _HTTPHANDLERS_h


class HttpHandlers {
    private:
        void addHandlerRoot();
        void addHandlerApiStatus();
        void addHandlerApiConfig();

        AsyncWebServer& _server;
        SystemManager& _systemManager;
        Config& _config;

    public:
        HttpHandlers(Config& config, AsyncWebServer& server, SystemManager& systemManager);
        void setup();
};

#endif
