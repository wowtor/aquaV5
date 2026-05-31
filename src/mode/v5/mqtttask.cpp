#include "mqtttask.h"

#include <esp_task_wdt.h>
#include <sstream>

#include "config.h"
#include "wifihandler.h"
#include "util.h"


namespace aquamqtt
{

/*** MQTTTASK PUBLIC ***/

MqttTask& MqttTask::getInstance() {
    static MqttTask instance(config::brokerAddr, config::brokerPort);
    return instance;
}

void MqttTask::queueUpdateEntity(Entity* entity)
{
    if (xSemaphoreTake(queue_mutex, portMAX_DELAY)) {
        tainted_entities.push(entity);
        xSemaphoreGive(queue_mutex);
    }
}

#ifdef MQTT_PUBLISH_FRAMES
void MqttTask::queueFrame(const Frame& frame) {
    if (xSemaphoreTake(queue_mutex, portMAX_DELAY)) {
        frame_queue.push(frame);
        xSemaphoreGive(queue_mutex);
    }
}

void MqttTask::queueDroppedBytes(const Frame& frame) {
    /*
    char hex[MAX_FRAME_SIZE * 2 + 1];
    for (int i=0 ; i<frame.get_buffer_size() ; i++) {
        sprintf(hex + 2*i, "%02X", frame.get_buffer()[i]);
    }
    LOG.print("dropping: ");
    LOG.println(hex);
    */

    if (xSemaphoreTake(queue_mutex, portMAX_DELAY)) {
        dropped_queue.push(frame);
        xSemaphoreGive(queue_mutex);
    }
}
#endif

void MqttTask::setup()
{
    Task::setup();

    client.begin(host, port, net);
    client.onMessage(MqttTask::messageReceived);
}

void MqttTask::loop()
{
    Task::loop();

    client.loop();

    if (!client.connected()) {
        connect();
        return;
    }

    if (!discovery_is_published) {
        publishDiscovery();
        discovery_is_published = true;
    }

    publishEntityStates();

    #ifdef MQTT_PUBLISH_FRAMES
    publishFrames();
    #endif

    const bool print_stats = (millis() - last_statistics_update_timestamp) >= 5000;
}

/*** MQTTTASK PRIVATE ***/

void MqttTask::messageReceived(const String& topic, const String& payload)
{
}

MqttTask::MqttTask(const char* _host, int _port)
    : Task("mqtt")
    , host(_host)
    , port(_port)
    , client(256)
{
    queue_mutex = xSemaphoreCreateMutex();
}

void MqttTask::connect()
{
    if (!WifiHandler::getInstance().connected()) {
        return;
    }

    LOG.println("[mqtt] trying to connect");
    while (!client.connect(
            config::brokerClientId,
            strlen(config::brokerUser) == 0 ? nullptr : config::brokerUser,
            strlen(config::brokerPassword) == 0 ? nullptr : config::brokerPassword))
    {
        //vTaskDelay(pdMS_TO_TICKS(1000));
        delay(1000);
        esp_task_wdt_reset();
    }
    LOG.println("[mqtt] connected");

    discovery_is_published = false;
}

void MqttTask::publishEntityState(Entity &entity)
{
    client.publish(entity.getStateTopic(), entity.state(), true, 0);
}

void MqttTask::publishEntityDiscovery(Entity &entity)
{
    char topic[100];
    snprintf(topic, 100, "homeassistant/sensor/%s/config", entity.getUniqueId());

    std::map<std::string,std::string> def = entity.definition();

    std::stringstream s;
    s << "{";

    // entity info
    for (auto it = def.begin(); it != def.end(); it++) {
        s << "\"" << it->first << "\":\"" << it->second << "\"";
        if (it != def.end()) {
            s << ",";
        }
    }

    // device info
    s << "\"dev\":{"; // device
    s << "\"ids\":\"" << entity.getDevice().getDeviceId() << "\""; // identifiers
    s << ",";
    s << "\"name\":\"" << entity.getDevice().getDeviceName() << "\"";
    s << "},";

    // origin info
    s << "\"o\":{";
    s << "\"name\":\"" << APP_NAME << "\"";
    s << "}";

    s << "}";

    LOG.print("[mqtt] discovery ");
    LOG.print(topic);
    LOG.print(": ");
    LOG.println(s.str().c_str());

    client.publish(topic, s.str().c_str(), true, 0);
}

void MqttTask::publishDiscovery() {
    DhwState& state = DhwState::getInstance();
    for (auto it = state.entities.begin() ; it != state.entities.end() ; it++) {
        publishEntityDiscovery(**it);
    }
}

void MqttTask::publishEntityStates() {
    if (xSemaphoreTake(queue_mutex, portMAX_DELAY)) {
        while (!tainted_entities.empty()) {
            publishEntityState(*tainted_entities.front());
            tainted_entities.pop();
        }

        xSemaphoreGive(queue_mutex);
    }
}

#ifdef MQTT_PUBLISH_FRAMES
void publishFrameQueue(MQTTClient& client, std::queue<Frame>& queue, const char* queue_name) {
    while (!queue.empty()) {
        auto frame = queue.front();

        char topic[100];
        snprintf(topic, 100, "%s/debug/%s/%s", unique_device_id(), queue_name, channel_name(frame.getChannel()));

        char hex[MAX_FRAME_SIZE * 2 + 1];
        for (int i=0 ; i<frame.get_buffer_size() ; i++) {
            sprintf(hex + 2*i, "%02X", frame.get_buffer()[i]);
        }
        client.publish(topic, hex);

        queue.pop();
    }
}

void MqttTask::publishFrames() {
    if (xSemaphoreTake(queue_mutex, portMAX_DELAY)) {
        publishFrameQueue(client, frame_queue, "frame");
        publishFrameQueue(client, dropped_queue, "dropped");
        xSemaphoreGive(queue_mutex);
    }
}
#endif // ifdef MQTT_PUBLISH_FRAMES

}  // namespace aquamqtt
