#include <ESPAsyncWebServer.h>
#include "WebSocketHandler.h"
#include "SystemManager.h"

#define DEBUG true  //set to true for debug output, false for no debug output
#define DEBUG_PRINT if(DEBUG)Serial
#define DEBUG_PRINTF if(DEBUG)printf

WebSocketHandler::WebSocketHandler(AsyncWebServer &server, AsyncWebSocket &webSocket, SystemManager &systemManager) :
    _server(server), _webSocket(webSocket), _systemManager(systemManager) {}

void WebSocketHandler::setup() {
    _webSocket.onEvent([this](AsyncWebSocket *s, AsyncWebSocketClient *c, AwsEventType type, void *a, uint8_t *data, size_t l) {
        if (type == WS_EVT_CONNECT) {
            DEBUG_PRINTF("WebSocket Connected");
        }

        if (type == WS_EVT_DISCONNECT) {
            DEBUG_PRINTF("Client disconnected from web socket\n");
        }
    });

    _server.addHandler(&_webSocket);
}

