#include "api.h"
#include "AsyncJson.h"
#include "ArduinoJson.h"

ApiController::ApiController(GlobalContext *context) : server(context->getServer()), context(context)
{
    // Initialize API endpoints here
    // Example:
    // server->on("/api/endpoint", HTTP_GET, [](AsyncWebServerRequest *request) {
    //     // Handle GET request
    // });
      AsyncCallbackJsonWebHandler *handler = new AsyncCallbackJsonWebHandler("/api/motors", 
      [](AsyncWebServerRequest *request, JsonVariant &json) {
        JsonObject jsonObj = json.as<JsonObject>();
        if (!jsonObj.containsKey("command")) {
            request->send(400, "application/json", "{\"error\":\"Missing command element\"}");
            return;
        }
        if (!jsonObj.containsKey("index")) {
            request->send(400, "application/json", "{\"error\":\"Missing command index\"}");
            return;
        }
      });
      server->addHandler(handler);

      // curl -X POST http://192.168.1.112/api/i2c-scan?index=0 
      server->on("/api/i2c-scan", HTTP_POST, [](AsyncWebServerRequest *request) {
        // get who param
        String indexStr;
        int index = 0;
        if (request->hasParam("index")) {
          indexStr = request->getParam("index")->value();
          if (indexStr.length() > 0) {
            index = indexStr.toInt();
          }
        }
        if (index < 0 || index > 1) {
          request->send(400, "text/plain", "Invalid bus index " + indexStr);
          return;
        }
        ctx()->i2c()->scanI2CBus(index);
        request->send(200, "text/plain", "I2c scan started for bus " + index);
      });
}
