#include <Arduino.h>
#include <esp_task_wdt.h>
#include <Preferences.h>

#include "config.h"
#include "wifihandler.h"
#include "serialtask.h"
#include "mqtttask.h"
#include "otahandler.h"


using namespace aquamqtt;
using namespace aquamqtt::config;

esp_task_wdt_config_t twdt_config = {
    .timeout_ms     = WATCHDOG_TIMEOUT_MS,
    .idle_core_mask = (1 << configNUM_CORES) - 1,
    .trigger_panic  = true,
};


OTAHandler otaHandler;


void loop()
{
    // watchdog
    esp_task_wdt_reset();
    delay(1);

    // handle wifi events
    WifiHandler::getInstance().loop();

    // handle over-the-air module in main thread
    otaHandler.loop();

    // handle mqtt client
    MqttTask::getInstance().loop();

    // handle serial interfaces
    if (OPERATION_MODE == OperationMode::LISTENER) {
        SerialTask::getListenerInstance().loop();
    } else {
        SerialTask::getHMIInstance().loop();
        SerialTask::getControllerInstance().loop();
    }
}

void setup()
{
    // limited serial output for debuggability
    Serial.begin(9600);
    Serial.println("REBOOT");

    // initialize watchdog
    esp_task_wdt_deinit();
    esp_task_wdt_init(&twdt_config);
    esp_task_wdt_add(nullptr);

    // setup wifi
    WifiHandler::getInstance().setup();

    // setup ota module
    otaHandler.setup();

    // setup mqtt client
    MqttTask::getInstance().setup();

    // setup serial interfaces
    if (OPERATION_MODE == OperationMode::LISTENER) {
        SerialTask::getListenerInstance().setup();
    } else {
        SerialTask::getHMIInstance().setup();
        SerialTask::getControllerInstance().setup();
    }
}
