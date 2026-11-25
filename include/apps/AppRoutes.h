#pragma once

#ifndef _APPROUTES_h
#define _APPROUTES_h

#include <ESPAsyncWebServer.h>
#include "apps/AppManager.h"

/// /api/apps: the apps with their settings, switching the active one, changing settings.
void addAppRoutes(AsyncWebServer& server, AppManager& apps);

#endif
