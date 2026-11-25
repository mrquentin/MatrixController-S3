#include "net/HttpJson.h"

#include "Log.h"

// Mozilla's root certificates, built into the SDK's mbedTLS
extern const uint8_t rootCertificates[] asm("_binary_x509_crt_bundle_start");

namespace {
    int requestJson(const String& url, const String* body, JsonDocument& doc, const JsonDocument* filter,
                    const HttpOptions& options) {
        HttpRequest request;
        const int status = request.send(url, options, body);
        if (status != HTTP_CODE_OK) return status;

        const DeserializationError error = filter
            ? deserializeJson(doc, request.body(), DeserializationOption::Filter(*filter))
            : deserializeJson(doc, request.body());
        if (error) {
            LOGF("HTTP: invalid JSON from %s: %s\n", url.c_str(), error.c_str());
            return HttpInvalidJson;
        }
        return HTTP_CODE_OK;
    }
}

int HttpRequest::send(const String& url, const HttpOptions& options, const String* body) {
    const bool https = url.startsWith("https://");
    if (https) {
        if (options.verifyCertificate) {
            _secure.setCACertBundle(rootCertificates);
        } else {
            _secure.setInsecure();
        }
    }

    _http.setTimeout(options.timeoutMs);
    _http.setConnectTimeout(options.timeoutMs);
    // HTTP/1.0 rules out chunked responses, so the body can be read straight from the stream
    _http.useHTTP10(true);
    if (options.followRedirects) _http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    if (options.userAgent) _http.setUserAgent(options.userAgent);

    if (!(https ? _http.begin(_secure, url) : _http.begin(_plain, url))) {
        LOGF("HTTP: invalid URL %s\n", url.c_str());
        return HttpInvalidUrl;
    }
    if (options.accept) _http.addHeader("Accept", options.accept);

    int status;
    if (body) {
        _http.addHeader("Content-Type", "application/json");
        status = _http.POST(*body);
    } else {
        status = _http.GET();
    }
    if (status != HTTP_CODE_OK) {
        LOGF("HTTP: %s %s failed: %d %s\n", body ? "POST" : "GET", url.c_str(), status,
             HTTPClient::errorToString(status).c_str());
    }
    return status;
}

int fetchJson(const String& url, JsonDocument& doc, const JsonDocument* filter, const HttpOptions& options) {
    return requestJson(url, nullptr, doc, filter, options);
}

int postJson(const String& url, const String& body, JsonDocument& doc, const JsonDocument* filter,
             const HttpOptions& options) {
    return requestJson(url, &body, doc, filter, options);
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
