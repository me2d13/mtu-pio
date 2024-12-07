#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "SPIFFS.h"
#include "Logger.h"
#include "web.h"
#include <string>
#include <vector>

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

// Structure to hold page information
struct Page {
    std::string name;
    std::string title;
    std::string link;
};

// Define the pages
std::vector<Page> pages = {
    {"log", "Logs", "/log"},
    {"motor", "Motor Control", "/motor"},
    {"about", "About", "/about"}
};

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

std::string generateNav(const std::string& currentPage) {
    std::string nav = "";
    for (const auto& page : pages) {
        if (page.name == currentPage) {
            nav += "<li><b>" + page.title + "</b></li>";
        } else {
            nav += "<li><a href=\"" + page.link + "\">" + page.title + "</a></li>";
        }
    }
    return nav;
}

void servePage(
    AsyncWebServerRequest* request,
    const std::string& title,
    const std::string& body,
    const std::string& currentPage
    ) {
    AsyncResponseStream *response = request->beginResponseStream("text/html");
    response->print(html1);
    response->printf("<title>%s</title>", title.c_str());
    response->print(html2);
    response->printf("<header>%s</header>", title.c_str());
    response->print(html3);
    response->print(generateNav(currentPage).c_str());
    response->print(html4);
    response->print(body.c_str());
    response->print(html5);
    request->send(response);
}

void handleLogRequest(AsyncWebServerRequest* request) {
    std::string body = "<h1>Log Entries</h1><ul id=\"logs\">";
    std::deque<std::string> logs = logger.getLogs();
    for (const auto& log : logs) {
        body += "<li>";
        body += log;
        body += "</li>\n";
    }
    body += "</ul>";
    servePage(request, "Logs", body, "log");
}

void handleMotorRequest(AsyncWebServerRequest* request) {
    File file = SPIFFS.open("/motor.html", "r");
    if (!file.available()) {
        request->send(500, "text/plain", "Failed to open motor.html");
        return;
    }
    String body = file.readString();
    file.close();
    servePage(request, "Motor Control", std::string(body.c_str()), "motor");
}

void handleAboutRequest(AsyncWebServerRequest* request) {
    File file = SPIFFS.open("/about.html", "r");
    if (!file.available()) {
        request->send(500, "text/plain", "Failed to open about.html");
        return;
    }
    String body = file.readString();
    file.close();
    servePage(request, "About", std::string(body.c_str()), "about");
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
    server.on("/log", HTTP_GET, handleLogRequest);

    // Serve the motor page
    server.on("/motor", HTTP_GET, handleMotorRequest);

    // Serve the about page
    server.on("/about", HTTP_GET, handleAboutRequest);

    server.onNotFound(notFound);

    // Attach WebSocket to server
    ws.onEvent(onWebSocketEvent);
    server.addHandler(&ws);

    server.begin();
}
