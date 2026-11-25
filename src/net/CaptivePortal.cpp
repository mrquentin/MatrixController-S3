#include "net/CaptivePortal.h"

#include <WiFi.h>
#include "net/WifiRoutes.h"
#include "net/WifiStore.h"
#include "Log.h"

namespace {
    constexpr uint16_t DnsPort = 53;
    // Long enough for the answer to reach the phone before the network goes away
    constexpr uint32_t RestartDelayMs = 2500;

    // Plain page with no external assets: there is no internet on the setup network, and
    // captive portal browsers are limited
    const char PortalPage[] PROGMEM = R"HTML(<!doctype html>
<html><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><meta name="color-scheme" content="light dark">
<title>Matrix Controller setup</title>
<style>
*{box-sizing:border-box}body{margin:0;font:16px -apple-system,system-ui,sans-serif;background:#f2f2f7;color:#1c1c1e}
main{max-width:420px;margin:0 auto;padding:24px 16px}h1{font-size:28px;margin:8px 0 4px}p{color:#6c6c70;margin:0 0 20px}
.card{background:#fff;border-radius:12px;padding:4px 16px;margin-bottom:20px}
label{display:block;font-size:13px;color:#6c6c70;margin:12px 0 4px}
input{width:100%;font:inherit;padding:10px 0;border:0;border-bottom:1px solid #e5e5ea;outline:0;background:none;color:inherit}
.nets button{display:flex;justify-content:space-between;width:100%;font:inherit;text-align:left;padding:12px 0;border:0;border-bottom:1px solid #e5e5ea;background:none;color:inherit}
.nets button:last-child{border:0}.muted{color:#8e8e93}
.save{width:100%;font:inherit;font-weight:600;padding:14px;border:0;border-radius:12px;background:#007aff;color:#fff}
.show{font-size:14px;color:#007aff;background:none;border:0;padding:8px 0}
@media (prefers-color-scheme:dark){body{background:#000;color:#fff}.card{background:#1c1c1e}input,.nets button{border-color:#38383a}}
</style></head><body><main>
<h1>WiFi setup</h1><p>Choose the network the Matrix Controller should join.</p>
<div class="card nets" id="nets"><div class="muted" style="padding:12px 0">Looking for networks...</div></div>
<form method="post" action="/save"><div class="card">
<label for="ssid">Network name</label><input id="ssid" name="ssid" maxlength="32" required autocapitalize="off" autocorrect="off">
<label for="password">Password</label><input id="password" name="password" type="password" maxlength="63">
<button type="button" class="show" onclick="var p=document.getElementById('password');p.type=p.type=='password'?'text':'password'">Show password</button>
</div><button class="save">Save and restart</button></form>
</main><script>
function esc(s){return s.replace(/[&<>"]/g,function(c){return{'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;'}[c]})}
function scan(){fetch('/api/system/wifi/scan').then(function(r){return r.json()}).then(function(list){
if(!list.length){setTimeout(scan,2000);return}
document.getElementById('nets').innerHTML=list.map(function(n){return '<button type="button" data-ssid="'+esc(n.ssid)+'"><span>'+esc(n.ssid)+'</span><span class="muted">'+(n.secure?'&#128274; ':'')+n.quality+'%</span></button>'}).join('');
document.querySelectorAll('#nets button').forEach(function(b){b.onclick=function(){document.getElementById('ssid').value=b.dataset.ssid;document.getElementById('password').focus()}})
}).catch(function(){setTimeout(scan,3000)})}
scan();
</script></body></html>)HTML";

    String escapeHtml(const String& text) {
        String escaped;
        for (const char c : text) {
            switch (c) {
                case '&': escaped += "&amp;"; break;
                case '<': escaped += "&lt;"; break;
                case '>': escaped += "&gt;"; break;
                case '"': escaped += "&quot;"; break;
                default: escaped += c;
            }
        }
        return escaped;
    }

    String messagePage(const String& title, const String& text) {
        return "<!doctype html><html><head><meta charset=\"utf-8\"><meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
               "<style>body{margin:0;font:16px -apple-system,system-ui,sans-serif;background:#f2f2f7;color:#1c1c1e}"
               "main{max-width:420px;margin:0 auto;padding:24px 16px}p{color:#6c6c70}a{color:#007aff}"
               "@media (prefers-color-scheme:dark){body{background:#000;color:#fff}}</style></head><body><main><h1>" +
               title + "</h1><p>" + text + "</p></main></body></html>";
    }
}

CaptivePortal::CaptivePortal(AsyncWebServer& server, SystemManager& system) : _server(server), _system(system) {}

void CaptivePortal::begin(const std::string& ssid, const std::string& password) {
    // AP + station: the station side is only used to scan for networks to offer
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(ssid.c_str(), password.c_str());
    LOGF("Setup network \"%s\" started at %s\n", ssid.c_str(), WiFi.softAPIP().toString().c_str());

    _dns.setErrorReplyCode(DNSReplyCode::NoError);
    _dns.start(DnsPort, "*", WiFi.softAPIP());

    // Results are usually ready by the time a phone opens the page
    WiFi.scanNetworks(true);

    addRoutes();
}

void CaptivePortal::loop() {
    _dns.processNextRequest();
}

int CaptivePortal::connectedDevices() const {
    return WiFi.softAPgetStationNum();
}

void CaptivePortal::addRoutes() {
    _server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->send(200, "text/html", PortalPage);
    });

    addWifiScanRoute(_server);

    _server.on("/save", HTTP_POST, [this](AsyncWebServerRequest* request) {
        const AsyncWebParameter* ssidParam = request->getParam("ssid", true);
        const AsyncWebParameter* passwordParam = request->getParam("password", true);
        WifiCredentials credentials;
        if (ssidParam) credentials.ssid = ssidParam->value().c_str();
        if (passwordParam) credentials.password = passwordParam->value().c_str();

        if (!WifiStore::isValid(credentials)) {
            request->send(400, "text/html", messagePage("Check the details",
                String(WifiStore::ValidityRule) + ". <a href=\"/\">Try again</a>"));
            return;
        }

        WifiStore::save(credentials);
        LOGF("WiFi credentials saved for \"%s\"\n", credentials.ssid.c_str());
        request->send(200, "text/html", messagePage("Saved",
            "The Matrix Controller restarts and joins <b>" + escapeHtml(credentials.ssid.c_str()) +
            "</b>. This setup network will disappear; you can reconnect your phone to your usual WiFi."));
        _system.scheduleRestart(credentials.ssid, RestartDelayMs);
    });

    // Every other address (including the phones' "is there internet?" checks) leads to the portal,
    // which is what makes them show the sign-in page
    _server.onNotFound([](AsyncWebServerRequest* request) {
        request->redirect("http://" + WiFi.softAPIP().toString() + "/");
    });
}
