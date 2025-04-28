#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
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
    bool hasJavascipt;

    Page(const std::string& name, const std::string& title, const std::string& link, bool hasJavascipt)
        : name(name), title(title), link(link), hasJavascipt(hasJavascipt) {}
};

// Define the pages
std::vector<Page> pages = {
    Page("log", "Logs", "/log", false),
    Page("motor", "Motor Control", "/motor", true),
    Page("config", "Configuration", "/config", true),
    Page("about", "About", "/about", false)
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
    std::string nav = "<li><strong>MTU</strong></li>";
    nav += "</ul><ul>";
    for (const auto& page : pages) {
        if (page.name == currentPage) {
            nav += "<li><button class=\"secondary\">" + page.title + "</button></li>";
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
    const std::string& currentPage,
    const std::string& jsFile = ""
    ) {
    AsyncResponseStream *response = request->beginResponseStream("text/html");
    response->print(html1);
    response->printf("<title>%s</title>", title.c_str());
    if (!jsFile.empty()) {
        response->printf("<script src=\"%s\"></script>", jsFile.c_str());
    }
    response->print(html2);
    //response->printf("<header>%s</header>", title.c_str());
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
    std::string jsPage = page->hasJavascipt ? page->name + ".js" : "";
    servePage(request, page->title, std::string(body.c_str()), page->name, jsPage);
}

AsyncWebServer* setupWeb() {
    server.serveStatic("/", SPIFFS, "/www/");

    server.on("/heap", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(200, "text/plain", String(ESP.getFreeHeap()));
    });

    server.on("/hw", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(200, "text/plain", "Hello, world");
    });

    // Serve the log page
    server.on("/log", HTTP_GET, handleLogRequest);

    // serve all other pages
    for (const auto& page : pages) {
        if (page.name != "log") {
            server.on(page.link.c_str(), HTTP_GET, [page](AsyncWebServerRequest* request) {
            handlePageRequest(request, &page);
            });
        }
    }

    server.onNotFound(notFound);

    // Attach WebSocket to server
    ws.onEvent(onWebSocketEvent);
    server.addHandler(&ws);

    server.begin();
    logger.log("HTTP server started");
    return &server;
}
