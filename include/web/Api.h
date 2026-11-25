#pragma once

#ifndef _WEBAPI_h
#define _WEBAPI_h

#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

/// Answers for the JSON API. Errors look like {"result": "error", "details": "..."}, which the web UI
/// shows as they are.
void sendJson(AsyncWebServerRequest* request, const JsonDocument& doc, int status = 200);
/// {"result": "success"}
void sendOk(AsyncWebServerRequest* request, int status = 200);
void sendError(AsyncWebServerRequest* request, int status, const char* details);

/// Lets the web UI's development server (another origin) call the API. Takes the server's not-found
/// handler, which answers the browsers' preflight requests.
void allowCrossOrigin(AsyncWebServer& server);

#endif
