#ifndef AQUAMQTT_SERIALTASK_H
#define AQUAMQTT_SERIALTASK_H

#include <queue>

#include <FastCRC.h>

#include "serialtask.h"
#include "framebuffer.h"
#include "frame.h"
#include "Task.h"

namespace aquamqtt
{

class SerialTask final : public Task
{
private:
    FrameChannel _channel;
    HardwareSerial *_port;
    uint8_t _gpio_rx;
    uint8_t _gpio_tx;
    uint8_t _gpio_enable_tx;

    FrameBuffer buffer;

    std::queue<Frame>  write_queue;
    SemaphoreHandle_t  write_queue_mutex;

    unsigned long        last_statistics_update_timestamp;

    SerialTask(FrameChannel channel, HardwareSerial *port, const uint8_t gpio_rx, const uint8_t gpio_tx, const uint8_t gpio_enable_tx);
    ~SerialTask() = default;

    const char* getName() const;
    void writeQueuedMessages();

public:
    static SerialTask& getHMIInstance();
    static SerialTask& getControllerInstance();
    static SerialTask& getListenerInstance();

    SerialTask(SerialTask const&) = delete;
    void operator=(SerialTask const&) = delete;

    void queueSendFrame(const Frame &message);
    void sendByte(const uint8_t value);

    void setup() override;
    void loop() override;

protected:
    virtual void periodicUpdate() override;
};
}  // namespace aquamqtt

#endif  // AQUAMQTT_SERIALTASK_H
