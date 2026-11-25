#include "web/WebUi.h"

#include <ui/WebAssets.h>

void addWebUiRoutes(AsyncWebServer& server) {
    for (const WebAsset& asset : WebAssets) {
        server.on(asset.path, HTTP_GET, [&asset](AsyncWebServerRequest* request) {
            AsyncWebServerResponse* response = request->beginResponse(200, asset.contentType, asset.data, asset.length);
            if (asset.gzipped) response->addHeader("Content-Encoding", "gzip");
            request->send(response);
        });
    }

    static AsyncWebSocket socket("/ws");
    server.addHandler(&socket);
}
