#pragma once

#ifndef _HTTPJSON_h
#define _HTTPJSON_h

#include <Arduino.h>
#include <ArduinoJson.h>

/// GETs url and parses the JSON response into doc. Blocks until done (up to the timeout), so
/// call it from a background task such as PeriodicTask, never from render() or a web handler.
/// filter, if given, keeps only the fields it lists (see ArduinoJson's DeserializationOption::Filter).
/// https:// URLs are encrypted but the server certificate is not checked.
/// Returns false on network errors, non-200 responses and invalid JSON.
bool fetchJson(const String& url, JsonDocument& doc, const JsonDocument* filter = nullptr);

/// Percent-encodes text for use in a URL query parameter.
String urlEncode(const char* text);

#endif
