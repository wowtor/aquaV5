#ifndef AQUAMQTT_MQTTTASK_H
#define AQUAMQTT_MQTTTASK_H

#include <queue>

#include <FastCRC.h>
#include <WiFiClient.h>
#include <MQTTClient.h>

#include "frame.h"
#include "dhwstate.h"
#include "Task.h"

#define DROPPED_SIZE 160

namespace aquamqtt
{

class MqttTask final : public Task
{
private:
    const char* host;
    int port;

    bool discovery_is_published = false;

    SemaphoreHandle_t   queue_mutex;
    std::queue<Entity*> tainted_entities;
    std::queue<Frame>   frame_queue;
    std::queue<Frame>   dropped_queue;

    unsigned long       last_statistics_update_timestamp = 0;

    WiFiClient net;
    MQTTClient client;

public:
    static MqttTask& getInstance();

    MqttTask(MqttTask const&) = delete;
    void operator=(MqttTask const&) = delete;

    void queueUpdateEntity(Entity* entity);

    #ifdef MQTT_PUBLISH_FRAMES
    void queueFrame(const Frame& frame);
    void queueDroppedBytes(const Frame& frame);
    #endif

    void setup() override;
    void loop() override;

private:
    static void messageReceived(const String& topic, const String& payload);

    MqttTask(const char* host, int port);
    ~MqttTask() = default;

    void connect();
    void publishEntityState(Entity &entity);
    void publishEntityDiscovery(Entity &entity);

    void publishDiscovery();
    void publishEntityStates();
    void publishFrames();
};

}  // namespace aquamqtt

#endif  // AQUAMQTT_MQTTTASK_H
