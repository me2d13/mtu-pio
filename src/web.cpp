#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include "Logger.h"
#include "web.h"
#include <string>
#include <vector>
#include <map>
#include <ArduinoJson.h>
#include "esp32/clk.h"
#include "net.h"
#include "context.h"

AsyncWebServer server(80);
AsyncWebSocket ws("/ws"); // WebSocket on "/ws"

// Whole page shell. Placeholders in order: title, nav, body, script tags.
const char* pageTemplate = R"rawliteral(<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="utf-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1" />
  <title>%s</title>
  <link rel="stylesheet" href="/style.css" />
</head>
<body>
  <header class="topbar">
    <a class="brand" href="/">MTU<small>motorized throttle unit</small></a>
    <nav class="nav">%s</nav>
  </header>
  <main>
    <article class="panel">
      %s
    </article>
  </main>
  <footer class="footbar">
    <span>ESP32-S3 throttle quadrant</span>
    <span class="chip"><span class="dot" id="conn-dot"></span><span id="conn-text">offline</span></span>
  </footer>
  %s
</body>
</html>
)rawliteral";

// Structure to hold page information
struct Page {
    std::string name;
    std::string title;
    std::string link;
    bool hasJavascript;

    Page(const std::string& name, const std::string& title, const std::string& link, bool hasJavascript)
        : name(name), title(title), link(link), hasJavascript(hasJavascript) {}
};

// Define the pages, in navigation order
std::vector<Page> pages = {
    Page("home", "MTU - Home", "/", false),
    Page("state", "MTU - State", "/state", true),
    Page("sim", "MTU - Simulator", "/sim", true),
    Page("logs", "MTU - Logs", "/logs", true),
    Page("settings", "MTU - Settings", "/settings", true),
    Page("motors", "MTU - Motors", "/motors", true),
    Page("debug", "MTU - Debug", "/debug", true)
};

// Old urls keep working
const std::map<std::string, std::string> legacyRedirects = {
    {"/log", "/logs"},
    {"/sensors", "/state"},
    {"/config", "/settings"},
    {"/motor", "/motors"},
    {"/about", "/debug"}
};

// Per client position in the log ring, so each client only gets entries it has not seen
std::map<uint32_t, uint32_t> logCursor;

std::string escapeHtml(const std::string& text) {
    std::string out;
    out.reserve(text.size() + 8);
    for (char c : text) {
        switch (c) {
            case '&': out += "&amp;"; break;
            case '<': out += "&lt;"; break;
            case '>': out += "&gt;"; break;
            case '"': out += "&quot;"; break;
            default: out += c;
        }
    }
    return out;
}

std::string formatUptime(unsigned long ms) {
    unsigned long seconds = ms / 1000;
    char buf[24];
    snprintf(buf, sizeof(buf), "%02lu:%02lu:%02lu", (seconds / 3600) % 100, (seconds / 60) % 60, seconds % 60);
    return std::string(buf);
}

std::string chip(const std::string& label, const std::string& value, const char* dotClass = nullptr) {
    std::string out = "<span class=\"chip\">";
    if (dotClass) out += "<span class=\"dot " + std::string(dotClass) + "\"></span>";
    out += label + " <b>" + escapeHtml(value) + "</b></span>";
    return out;
}

std::string logsJson(const std::vector<std::string>& entries, const char* type) {
    JsonDocument doc;
    doc["type"] = type;
    JsonArray arr = doc["data"].to<JsonArray>();
    for (const auto& entry : entries) arr.add(entry);
    std::string out;
    serializeJson(doc, out);
    return out;
}

std::string logListItem(const std::string& entry) {
    size_t pos = entry.find(" - ");
    std::string stamp = pos == std::string::npos ? "" : entry.substr(0, pos);
    std::string msg = pos == std::string::npos ? entry : entry.substr(pos + 3);
    bool warn = msg.find("WARN") != std::string::npos;
    return std::string("<li") + (warn ? " class=\"warn\"" : "") + "><span class=\"stamp\">" + escapeHtml(stamp) + "</span><span class=\"msg\">" + escapeHtml(msg) + "</span></li>";
}

void sendLogSnapshot(AsyncWebSocketClient* client) {
    std::vector<std::string> entries;
    for (const auto& entry : logger.getLogs()) entries.push_back(entry);
    client->text(logsJson(entries, "logs").c_str());
}

// WebSocket event handler
void onWebSocketEvent(AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len) {
    if (type == WS_EVT_CONNECT) {
        Serial.printf("WebSocket client connected: %u\n", client->id());
        logCursor[client->id()] = logger.total();
        sendLogSnapshot(client);
        client->text("{\"type\":\"state\",\"data\":" + ctx()->state.transient.reportState() + "}");
    } else if (type == WS_EVT_DISCONNECT) {
        Serial.printf("WebSocket client disconnected: %u\n", client->id());
        logCursor.erase(client->id());
    }
}

void notFound(AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "Not found, sorry");
}

std::string generateNav(const std::string& currentPage) {
    std::string nav;
    for (const auto& page : pages) {
        nav += "<a href=\"" + page.link + "\"" + (page.name == currentPage ? " aria-current=\"page\"" : "") + ">" + page.title + "</a>";
    }
    return nav;
}

void servePage(
    AsyncWebServerRequest* request,
    const std::string& title,
    const std::string& body,
    const std::string& currentPage,
    const std::string& jsFile = ""
    ) {
    std::string script = "<script src=\"/app.js\"></script>";
    if (!jsFile.empty()) script += "<script src=\"/" + jsFile + "\"></script>";
    AsyncResponseStream *response = request->beginResponseStream("text/html");
    response->printf(pageTemplate, title.c_str(), generateNav(currentPage).c_str(), body.c_str(), script.c_str());
    request->send(response);
}

std::string buildDashboardBody() {
    PersistedState& persisted = ctx()->state.persisted;
    TransientState& transient = ctx()->state.transient;

    std::string body = "<h1>Motorized throttle unit</h1>";
    body += "<p class=\"lede\">ESP32-S3 throttle quadrant - USB HID joystick with simulator driven motors.</p>";

    body += "<div class=\"chips\">";
    body += chip("uptime", formatUptime(millis()), "on");
    body += chip("last reset", resetReason());
    body += chip("heap", std::to_string(ESP.getFreeHeap() / 1024) + " KB");
    body += chip("min heap", std::to_string(ESP.getMinFreeHeap() / 1024) + " KB");
    body += chip("CPU", std::to_string(esp_clk_cpu_freq() / 1000000) + " MHz");
    body += chip("IP", getIp());
    body += chip("HID", persisted.isHidOn ? "on" : "off", persisted.isHidOn ? "on" : "off");
    body += chip("12V", transient.is12Vpresent() ? "present" : "absent", transient.is12Vpresent() ? "on" : "warn");
    body += "</div>";

    body += "<h2>Motors</h2><div class=\"chips\">";
    for (int i = 0; i < MOTORS_COUNT; i++) {
        bool enabled = true;
        if (i == MOTOR_INDEX_TRIM) enabled = persisted.enableTrimWheel;
        if (i == MOTOR_INDEX_SPEED_BRAKE) enabled = persisted.enableSpeedBrake;
        body += chip(persisted.motorSettings[i].name, enabled ? "enabled" : "disabled", enabled ? "on" : "off");
    }
    body += "</div>";

    body += "<div class=\"cards\">";
    body += "<a class=\"card\" href=\"/state\"><strong>State</strong><span>axes, buttons and encoder readings</span></a>";
    body += "<a class=\"card\" href=\"/sim\"><strong>Simulator</strong><span>what the unit is receiving from the sim</span></a>";
    body += "<a class=\"card\" href=\"/logs\"><strong>Logs</strong><span>what the firmware is doing</span></a>";
    body += "<a class=\"card\" href=\"/settings\"><strong>Settings</strong><span>persisted configuration</span></a>";
    body += "<a class=\"card\" href=\"/motors\"><strong>Motors</strong><span>drive the steppers directly</span></a>";
    body += "<a class=\"card\" href=\"/debug\"><strong>Debug</strong><span>i2c scan, hw calls, heap</span></a>";
    body += "</div>";

    return body;
}

void handleLogsRequest(AsyncWebServerRequest* request) {
    std::string body = "<h1>Logs</h1><p class=\"lede\">Newest entry at the top. Showing the last " + std::to_string(logger.size()) + " entries; new ones are streamed in over WebSocket.</p>";
    body += "<div class=\"chips\">";
    body += chip("free heap", std::to_string(ESP.getFreeHeap() / 1024) + " KB");
    body += chip("min heap", std::to_string(ESP.getMinFreeHeap() / 1024) + " KB");
    body += chip("CPU", std::to_string(esp_clk_cpu_freq() / 1000000) + " MHz");
    body += "</div>";
    body += "<ul class=\"logs\" id=\"logs\">";
    auto logs = logger.getLogs();
    for (auto it = logs.rbegin(); it != logs.rend(); ++it) body += logListItem(*it);
    body += "</ul>";
    servePage(request, "MTU - Logs", body, "logs", "logs.js");
}

void handlePageRequest(AsyncWebServerRequest* request, const Page* page) {
    if (page == nullptr) {
        request->send(404, "text/plain", "Page not found");
        return;
    }
    std::string htmlPage = "/www/" + page->name + ".html";
    File file = SPIFFS.open(htmlPage.c_str(), "r");
    if (!file.available()) {
        request->send(500, "text/plain", "Failed to open page file");
        return;
    }
    String body = file.readString();
    file.close();
    std::string jsPage = page->hasJavascript ? page->name + ".js" : "";
    servePage(request, page->title, std::string(body.c_str()), page->name, jsPage);
}

AsyncWebServer* setupWeb() {
    // Explicit routes before the static handler, so "/" resolves to the dashboard
    server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
        servePage(request, "MTU - Home", buildDashboardBody(), "home");
    });

    server.on("/logs", HTTP_GET, handleLogsRequest);

    for (const auto& page : pages) {
        if (page.name == "home" || page.name == "logs") continue;
        server.on(page.link.c_str(), HTTP_GET, [page](AsyncWebServerRequest* request) {
            handlePageRequest(request, &page);
        });
    }

    for (const auto& redirect : legacyRedirects) {
        std::string target = redirect.second;
        server.on(redirect.first.c_str(), HTTP_GET, [target](AsyncWebServerRequest* request) {
            request->redirect(target.c_str());
        });
    }

    server.on("/heap", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(200, "text/plain", String(ESP.getFreeHeap()));
    });

    // css and js assets
    server.serveStatic("/", SPIFFS, "/www/");

    server.onNotFound(notFound);

    // Attach WebSocket to server
    ws.onEvent(onWebSocketEvent);
    server.addHandler(&ws);

    server.begin();
    logger.log("HTTP server started");
    return &server;
}

// Push live state to everyone and the log entries each client has not seen yet
void broadcastUpdates() {
    if (ws.count() == 0) {
        return;
    }

    // ESPAsyncWebServer never prunes clients whose socket has gone away, so do it
    // here; a stale entry reports a full queue and would block the whole broadcast.
    ws.cleanupClients();

    String state = "{\"type\":\"state\",\"data\":" + ctx()->state.transient.reportState() + "}";

    for (auto &client : ws.getClients()) {
        // A throttled or background browser tab stops draining the socket; sending
        // into a full queue would only burn heap, so skip that client this tick.
        if (client.status() != WS_CONNECTED || client.queueIsFull()) {
            continue;
        }

        client.text(state.c_str());

        auto it = logCursor.find(client.id());
        if (it == logCursor.end()) {
            continue;
        }
        std::vector<std::string> fresh = logger.getLogsSince(it->second);
        if (fresh.empty()) {
            continue;
        }
        it->second = logger.total();
        client.text(logsJson(fresh, "logDelta").c_str());
    }
}
