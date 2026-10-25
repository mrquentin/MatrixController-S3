#include "net/HttpJson.h"

#include <HTTPClient.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>

#define DEBUG true  //set to true for debug output, false for no debug output
#define DEBUG_PRINT if(DEBUG)Serial
#define DEBUG_PRINTF if(DEBUG)printf

namespace {
    /// Sends a GET (body == nullptr) or a JSON POST and parses the response.
    bool request(const String& url, const String* body, JsonDocument& doc, const JsonDocument* filter,
                 const uint16_t timeoutMs) {
        const bool https = url.startsWith("https://");
        WiFiClientSecure secureClient;
        WiFiClient plainClient;
        // Weather and similar public data isn't worth bundling and updating root certificates for
        if (https) secureClient.setInsecure();

        HTTPClient http;
        http.setTimeout(timeoutMs);
        http.setConnectTimeout(timeoutMs);
        // HTTP/1.0 rules out chunked responses, so the body can be parsed straight from the stream
        http.useHTTP10(true);

        if (!(https ? http.begin(secureClient, url) : http.begin(plainClient, url))) {
            DEBUG_PRINTF("HTTP: invalid URL %s\n", url.c_str());
            return false;
        }

        int status;
        if (body) {
            http.addHeader("Content-Type", "application/json");
            status = http.POST(*body);
        } else {
            status = http.GET();
        }
        if (status != HTTP_CODE_OK) {
            DEBUG_PRINTF("HTTP: %s %s failed: %d %s\n", body ? "POST" : "GET", url.c_str(), status,
                         http.errorToString(status).c_str());
            http.end();
            return false;
        }

        const DeserializationError error = filter
            ? deserializeJson(doc, http.getStream(), DeserializationOption::Filter(*filter))
            : deserializeJson(doc, http.getStream());
        http.end();

        if (error) {
            DEBUG_PRINTF("HTTP: invalid JSON from %s: %s\n", url.c_str(), error.c_str());
            return false;
        }
        return true;
    }
}

bool fetchJson(const String& url, JsonDocument& doc, const JsonDocument* filter, const uint16_t timeoutMs) {
    return request(url, nullptr, doc, filter, timeoutMs);
}

bool postJson(const String& url, const String& body, JsonDocument& doc, const JsonDocument* filter,
              const uint16_t timeoutMs) {
    return request(url, &body, doc, filter, timeoutMs);
}

String urlEncode(const char* text) {
    static constexpr char Hex[] = "0123456789ABCDEF";
    String encoded;
    for (const char* p = text; *p; ++p) {
        const auto c = static_cast<unsigned char>(*p);
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            encoded += static_cast<char>(c);
        } else {
            encoded += '%';
            encoded += Hex[c >> 4];
            encoded += Hex[c & 0x0F];
        }
    }
    return encoded;
}
