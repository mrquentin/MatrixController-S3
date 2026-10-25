#pragma once

#include <ESPAsyncWebServer.h>

#include "Config.h"
#include "SystemManager.h"
#include "apps/AppManager.h"
#include "ota/FirmwareUpdate.h"
#include "ota/NetworkUpload.h"
#include "ota/ReleaseUpdate.h"

#ifndef _HTTPHANDLERS_h
#define _HTTPHANDLERS_h


class HttpHandlers {
    private:
        void addHandlerRoot();
        void addHandlerApiStatus();
        void addHandlerApiConfig();
        void addHandlerApiApps();
        void addHandlerApiWifi();
        void addHandlerApiRestart();
        void addHandlerApiUpdate();

        AsyncWebServer& _server;
        SystemManager& _systemManager;
        Config& _config;
        AppManager& _appManager;
        FirmwareUpdate& _firmwareUpdate;
        NetworkUpload& _networkUpload;
        ReleaseUpdate& _releaseUpdate;

    public:
        HttpHandlers(Config& config, AsyncWebServer& server, SystemManager& systemManager, AppManager& appManager,
                     FirmwareUpdate& firmwareUpdate, NetworkUpload& networkUpload, ReleaseUpdate& releaseUpdate);
        void setup();
};

#endif
