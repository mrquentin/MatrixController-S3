#pragma once

#ifndef _HTTPJSON_h
#define _HTTPJSON_h

#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>

/// Default time allowed for connecting and for the response, each.
constexpr uint16_t DefaultHttpTimeoutMs = 10000;
/// What HttpRequest::send returns for a URL it can't use.
constexpr int HttpInvalidUrl = -100;
/// What fetchJson and postJson return when the answer came but isn't the JSON asked for.
constexpr int HttpInvalidJson = -101;

struct HttpOptions {
    uint16_t timeoutMs = DefaultHttpTimeoutMs;
    /// Checks an https server's certificate against the SDK's root certificates. Off by default:
    /// public data like the weather isn't worth bundling and updating certificates for.
    bool verifyCertificate = false;
    bool followRedirects = false;
    const char* userAgent = nullptr;
    const char* accept = nullptr;
};

/// An HTTP request whose answer is read straight from the network as it arrives. Blocks until the
/// headers are in (up to the timeout), so use it from a background task such as PeriodicTask,
/// never from render() or a web handler.
class HttpRequest {
public:
    /// Sends a GET, or a POST of body as application/json. Returns the HTTP status, HttpInvalidUrl,
    /// or a negative HTTPClient error (see HTTPClient::errorToString).
    int send(const String& url, const HttpOptions& options = {}, const String* body = nullptr);
    /// The answer's body, once send() returned 200
    [[nodiscard]] WiFiClient& body() { return *_http.getStreamPtr(); }
    /// Content-Length, -1 when the server didn't say
    [[nodiscard]] int size() { return _http.getSize(); }

    ~HttpRequest() { _http.end(); }

private:
    WiFiClientSecure _secure;
    WiFiClient _plain;
    HTTPClient _http;
};

/// GETs url and parses the JSON answer into doc. filter, if given, keeps only the fields it lists
/// (see ArduinoJson's DeserializationOption::Filter). Returns 200 on success, otherwise what
/// HttpRequest::send returned, or HttpInvalidJson.
int fetchJson(const String& url, JsonDocument& doc, const JsonDocument* filter = nullptr, const HttpOptions& options = {});

/// Same as fetchJson, but POSTs body as application/json (e.g. a GraphQL query).
int postJson(const String& url, const String& body, JsonDocument& doc, const JsonDocument* filter = nullptr,
             const HttpOptions& options = {});

/// Percent-encodes text for use in a URL query parameter.
String urlEncode(const char* text);

#endif
