#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "SPIFFS.h"
#include "Logger.h"
#include "web.h"
#include <string>

AsyncWebServer server(80);
AsyncWebSocket ws("/ws"); // WebSocket on "/ws"

const char* html1 = R"rawliteral(
<!DOCTYPE html>
<html lang="en">

<head>
    <meta charset="utf-8" />
    <meta name="viewport" content="width=device-width, initial-scale=1" />
)rawliteral";
// flowed by title and script
const char* html2 = R"rawliteral(
    <link rel="stylesheet" href="/pico.min.css">
</head>

<body>
)rawliteral";
// flowed by header
const char* html3 = R"rawliteral(
    <section id="content">
        <nav>
            <ul>
)rawliteral";
// flowed by nav items
const char* html4 = R"rawliteral(
            </ul>
        </nav>
        <article>
)rawliteral";
// flowed by content
const char* html5 = R"rawliteral(
        </article>
    </section>
    <footer>
        Footer
    </footer>
</body>
</html>
)rawliteral";

// Function to notify clients of a new log message
void notifyClients(const std::string& logMessage) {
    String message = String(logMessage.c_str());
    ws.textAll(message); // Broadcast the message to all connected clients
}

// WebSocket event handler
void onWebSocketEvent(AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len) {
    if (type == WS_EVT_CONNECT) {
        Serial.printf("WebSocket client connected: %u\n", client->id());
    } else if (type == WS_EVT_DISCONNECT) {
        Serial.printf("WebSocket client disconnected: %u\n", client->id());
    }
}

void notFound(AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "Not found, sorry");
}

std::string generateNav() {
    return "<li><a href=\"/log\">Log</a></li>"
     "<li><a href=\"/about\">About</a></li>";
}

void servePage(
    AsyncWebServerRequest* request,
    const std::string& title,
    const std::string& body
    ) {
    AsyncResponseStream *response = request->beginResponseStream("text/html");
    response->print(html1);
    response->printf("<title>%s</title>", title.c_str());
    response->print(html2);
    response->printf("<header>%s</header>", title.c_str());
    response->print(html3);
    response->print(generateNav().c_str());
    response->print(html4);
    response->print(body.c_str());
    response->print(html5);
    request->send(response);
}

void setupWeb() {
    server.serveStatic("/", SPIFFS, "/www/");

    server.on("/heap", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(200, "text/plain", String(ESP.getFreeHeap()));
    });

    server.on("/hw", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(200, "text/plain", "Hello, world");
    });

    // Serve the log page
    server.on("/log", HTTP_GET, [](AsyncWebServerRequest* request) {
        String html = R"rawliteral(
            <!DOCTYPE html>
            <html>
            <head>
                <title>ESP Logs</title>
                <script>
                    var ws = new WebSocket('ws://' + location.host + '/ws');
                    ws.onmessage = function(event) {
                        var logList = document.getElementById('logs');
                        var newLog = document.createElement('li');
                        newLog.textContent = event.data;
                        logList.appendChild(newLog);
                    };
                </script>
            </head>
            <body>
                <h1>Log Entries</h1><ul id="logs"></ul>
            </body>
            </html>
        )rawliteral";
        std::string body = "<h1>Log Entries</h1><ul id=\"logs\">";
        std::deque<std::string> logs = logger.getLogs();
        for (const auto& log : logs) {
            body += "<li>";
            body += log;
            body += "</li>\n";
        }
        body += "</ul>";
        servePage(request, "Logs", body);
    });

    server.onNotFound(notFound);

    // Attach WebSocket to server
    ws.onEvent(onWebSocketEvent);
    server.addHandler(&ws);

    server.begin();
}