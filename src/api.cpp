#include "api.h"
#include "AsyncJson.h"
#include "ArduinoJson.h"
#include "Logger.h"
#include "context.h"
#include "state.h"

// example of post request
// curl -X POST http://192.168.1.112/api/motors -H 'Content-Type: application/json' -d '{"command":"moveToPosition","index":0, "parameters":{"position":5000}}'
// curl -X POST http://192.168.1.112/api/state -H 'Content-Type: application/json' -d '{"trimWheelVelocity":10}'

ApiController::ApiController(GlobalContext *context) : server(context->getServer()), context(context)
{
      AsyncCallbackJsonWebHandler *handler = new AsyncCallbackJsonWebHandler("/api/motors", 
      [](AsyncWebServerRequest *request, JsonVariant &json) {
        JsonObject jsonObj = json.as<JsonObject>();
        if (!jsonObj["command"].is<String>()) {
            request->send(400, "application/json", "{\"error\":\"Missing command element\"}");
            return;
        }
        String command = jsonObj["command"].as<String>();
        if (command.equals("stopAll")) {
            ctx()->motorsController.stopAllMotors();
            request->send(200, "text/plain", "All motors stopped");
            return;
        }
        if (!jsonObj["index"].is<int>()) {
            request->send(400, "application/json", "{\"error\":\"Missing command index\"}");
            return;
        }
        // get command as string and index as int
        int index = jsonObj["index"].as<int>();
        ctx()->motorsController.handleApiCommand(index, command, request, jsonObj);
      });
      handler->setMethod(HTTP_POST);
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
        ctx()->i2c()->startScan(index);
        request->send(200, "text/plain", "I2c scan started for bus " + index);
      });

      // curl -X POST http://192.168.1.112/api/debug?index=1 
      server->on("/api/debug", HTTP_POST, [](AsyncWebServerRequest *request) {
        logger.log("Starting debug call");
        int index = 0;
        if (request->hasParam("index")) {
          String indexStr = request->getParam("index")->value();
          if (indexStr.length() > 0) {
            index = indexStr.toInt();
          }
        }
        ctx()->debugCall(index);
        logger.log("Debug call done");
        request->send(200, "text/plain", "Debug call done");
      });

      server->on("/api/state", HTTP_GET, [](AsyncWebServerRequest *request) {
        logger.log("Logging state on API request");
        String state = "{";
        state += "\"transient\":" + ctx()->state.transient.reportState() + ",";
        state += "\"persisted\":" + ctx()->state.persisted.reportState() + "}";
        request->send(200, "application/json", state);
        logger.log("Logging state done");
      });

      AsyncCallbackJsonWebHandler *configHandler = new AsyncCallbackJsonWebHandler("/api/state", 
        [](AsyncWebServerRequest *request, JsonVariant &json) {
          JsonObject jsonObj = json.as<JsonObject>();
          bool factoryReset = jsonObj["factoryReset"].as<bool>();
          if (factoryReset) {
            ctx()->state.persisted.factoryReset();
            request->send(200, "text/plain", "Factory reset done");
            return;
          }
          String result = ctx()->state.persisted.loadFromJsonObject(jsonObj, true);
          request->send(200, "text/plain", result);
        });
        configHandler->setMethod(HTTP_POST);
        server->addHandler(configHandler);
}
