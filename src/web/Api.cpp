#include "web/Api.h"

void sendJson(AsyncWebServerRequest* request, const JsonDocument& doc, const int status) {
    String response;
    serializeJson(doc, response);
    request->send(status, "application/json", response);
}

void sendOk(AsyncWebServerRequest* request, const int status) {
    request->send(status, "application/json", "{\"result\":\"success\"}");
}

void sendError(AsyncWebServerRequest* request, const int status, const char* details) {
    JsonDocument doc;
    doc["result"] = "error";
    doc["details"] = details;
    sendJson(request, doc, status);
}

void allowCrossOrigin(AsyncWebServer& server) {
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "GET,POST");
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "*");

    // Preflight requests, for every route
    server.onNotFound([](AsyncWebServerRequest* request) {
        request->send(request->method() == HTTP_OPTIONS ? 200 : 404);
    });
}
