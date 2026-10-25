#pragma once

#ifndef _HTTPJSON_h
#define _HTTPJSON_h

#include <Arduino.h>
#include <ArduinoJson.h>

/// Default time allowed for connecting and for the response, each.
constexpr uint16_t DefaultHttpTimeoutMs = 10000;

/// GETs url and parses the JSON response into doc. Blocks until done (up to the timeout), so
/// call it from a background task such as PeriodicTask, never from render() or a web handler.
/// filter, if given, keeps only the fields it lists (see ArduinoJson's DeserializationOption::Filter).
/// https:// URLs are encrypted but the server certificate is not checked.
/// Returns false on network errors, non-200 responses and invalid JSON.
bool fetchJson(const String& url, JsonDocument& doc, const JsonDocument* filter = nullptr,
               uint16_t timeoutMs = DefaultHttpTimeoutMs);

/// Same as fetchJson, but POSTs body as application/json (e.g. a GraphQL query).
bool postJson(const String& url, const String& body, JsonDocument& doc, const JsonDocument* filter = nullptr,
              uint16_t timeoutMs = DefaultHttpTimeoutMs);

/// Percent-encodes text for use in a URL query parameter.
String urlEncode(const char* text);

#endif
