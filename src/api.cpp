#include "api.h"
#include "AsyncJson.h"
#include "ArduinoJson.h"
#include "Logger.h"
#include "context.h"
#include "state.h"
#include "web.h"

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
            request->send(200, "text/plain", "Factory reset done. Don't forget to Save");
            return;
          }
          String result = ctx()->state.persisted.loadFromJsonObject(jsonObj, true);
          request->send(200, "text/plain", result);
        });
        configHandler->setMethod(HTTP_POST);
        server->addHandler(configHandler);

      // Mock simulator values - goes through the same parser and driver callbacks as a UDP packet.
      // curl -X POST http://mtu.local/api/sim -H 'Content-Type: application/json' \
      //   -d '{"throttle1":0.5,"throttle2":0.5,"speedBrake":1,"trim":5,"parkingBrake":true,"autoThrottle":true}'
      // A body in the simulator format works too:
      //   -d '{"sim/flightmodel/engine/ENGN_thro": [0.5, 0.5], "laminar/B738/flt_ctrls/speedbrake_lever": 1.0}'
      AsyncCallbackJsonWebHandler *simHandler = new AsyncCallbackJsonWebHandler("/api/sim",
        [](AsyncWebServerRequest *request, JsonVariant &json) {
          JsonObject data = json.as<JsonObject>();
          if (data.size() == 0) {
            request->send(400, "application/json", "{\"error\":\"Empty body - nothing to apply\"}");
            return;
          }

          String error = ctx()->simUdpInterface.applyMock(data);
          if (error.length() > 0) {
            request->send(400, "application/json", "{\"error\":\"" + error + "\"}");
            return;
          }

          sim_data *sim = ctx()->state.transient.getSimData();
          String result = "{\"applied\":{";
          result += "\"throttle1\":" + String(sim->throttle1, 3) + ",";
          result += "\"throttle2\":" + String(sim->throttle2, 3) + ",";
          result += "\"speedBrake\":" + String(sim->speedBrake, 3) + ",";
          result += "\"trim\":" + String(sim->trim, 2) + ",";
          result += "\"parkingBrake\":";
          result += (sim->parkingBrake ? "true" : "false");
          result += ",";
          result += "\"autoThrottle\":";
          result += (sim->autoThrottle ? "true" : "false");
          result += "}}";
          request->send(200, "application/json", result);
        });
        simHandler->setMethod(HTTP_POST);
        server->addHandler(simHandler);
}

void ApiController::setup() {
    // Setup periodic task to broadcast sensor data via WebSocket
    // 500ms interval as requested
    sensorBroadcastTask.set(500, TASK_FOREVER, []() {
        broadcastUpdates();
    });
    context->taskScheduler.addTask(sensorBroadcastTask);
    sensorBroadcastTask.enable();
    logger.log("Sensor broadcast task scheduled (500ms interval)");
}
