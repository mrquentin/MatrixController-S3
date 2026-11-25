#pragma once

#ifndef _SYSTEMROUTES_h
#define _SYSTEMROUTES_h

#include <ESPAsyncWebServer.h>
#include "Config.h"
#include "SystemManager.h"
#include "sensors/AutoOrientation.h"

/// /api/system/status, /api/system/config and /api/system/restart.
void addSystemRoutes(AsyncWebServer& server, SystemManager& system, Config& config, const AutoOrientation& orientation);

#endif
