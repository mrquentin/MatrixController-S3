#pragma once

#include <ESPAsyncWebServer.h>

#include "Config.h"
#include "SystemManager.h"
#include "apps/AppManager.h"

#ifndef _HTTPHANDLERS_h
#define _HTTPHANDLERS_h


class HttpHandlers {
    private:
        void addHandlerRoot();
        void addHandlerApiStatus();
        void addHandlerApiConfig();
        void addHandlerApiApps();

        AsyncWebServer& _server;
        SystemManager& _systemManager;
        Config& _config;
        AppManager& _appManager;

    public:
        HttpHandlers(Config& config, AsyncWebServer& server, SystemManager& systemManager, AppManager& appManager);
        void setup();
};

#endif
