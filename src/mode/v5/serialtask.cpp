#include "serialtask.h"

#include <esp_task_wdt.h>

#include "config.h"
#include "util.h"


#define WRITE_QUEUE_SIZE 8

namespace aquamqtt
{

/*** SERIALTASK PUBLIC ***/

SerialTask& SerialTask::getHMIInstance() {
    static SerialTask instance(FrameChannel::CH_HMI, &Serial1, config::GPIO_HMI_RX, config::GPIO_HMI_TX, config::GPIO_HMI_ENABLE_TX);
    return instance;
}

SerialTask& SerialTask::getControllerInstance() {
    static SerialTask instance(FrameChannel::CH_MAIN, &Serial2, config::GPIO_MAIN_RX, config::GPIO_MAIN_TX, config::GPIO_MAIN_ENABLE_TX);
    return instance;
}

SerialTask& SerialTask::getListenerInstance() {
    static SerialTask instance(FrameChannel::CH_LISTENER, &Serial2, config::GPIO_MAIN_RX, 0, 0);
    return instance;
}

void SerialTask::queueSendFrame(const Frame &frame) {
    if (xSemaphoreTake(write_queue_mutex, portMAX_DELAY)) {
        if (write_queue.size() >= WRITE_QUEUE_SIZE) {
            write_queue.pop();
        }
        write_queue.push(frame);

        xSemaphoreGive(write_queue_mutex);
    }
}

void SerialTask::sendByte(const uint8_t value) {
    digitalWrite(_gpio_enable_tx, HIGH);
    _port->write(value);
    _port->flush();
    digitalWrite(_gpio_enable_tx, LOW);
}

void SerialTask::spawn()
{
    TaskHandle_t handle;
    xTaskCreate(SerialTask::innerTask, getName(), 3000, this, 4, &handle);
    //esp_task_wdt_add(handle);
}

void SerialTask::setup()
{
    LOG.print("[serial] setup channel: ");
    LOG.println(getName());
    _port->begin(config::DEFAULT_SERIAL_BAUD, config::DEFAULT_SERIAL_CONFIGURATION, _gpio_rx, _gpio_tx);
    if (_gpio_enable_tx) {
        pinMode(_gpio_enable_tx, OUTPUT);
    }
}

void SerialTask::loop()
{
    const bool printSerialStats   = (millis() - last_statistics_update_timestamp) >= 5000;

    while (_port->available())
    {
        uint8_t value = _port->read();
        buffer.pushByte(value);
    }

    writeQueuedMessages();

    if (printSerialStats)
    {
        LOG.print("[");
        LOG.print(getName());
        LOG.print("] messages received: ");
        LOG.print(buffer.countFramesReceived());
        LOG.print("; checksums errors: ");
        LOG.print(buffer.countChecksumErrors());
        LOG.print("; long messages: ");
        LOG.print(buffer.countLongFrameErrors());
        LOG.print("; buffer full: ");
        LOG.print(buffer.countBufferFullErrors());
        LOG.print("; no magic: ");
        LOG.print(buffer.countNoMagicErrors());
        LOG.println();

        LOG.print("[serial] stack size: ");
        LOG.print(uxTaskGetStackHighWaterMark(nullptr));
        LOG.print("; minimum ever heep size: ");
        LOG.println(xPortGetMinimumEverFreeHeapSize());

        last_statistics_update_timestamp = millis();
    }
}

/*** SERIALTASK PRIVATE ***/

[[noreturn]] void SerialTask::innerTask(void* pvParameters)
{
    auto* self = (SerialTask*) pvParameters;

    self->setup();
    while (true)
    {
        //esp_task_wdt_reset();
        self->loop();
    }
}

SerialTask::SerialTask(FrameChannel channel, HardwareSerial *port, const uint8_t gpio_rx, const uint8_t gpio_tx, const uint8_t gpio_enable_tx)
    : _channel(channel)
    , _port((HardwareSerial*)port)
    , _gpio_rx(gpio_rx)
    , _gpio_tx(gpio_tx)
    , _gpio_enable_tx(gpio_enable_tx)
    , buffer(channel)
    , last_statistics_update_timestamp(0)
{
    write_queue_mutex = xSemaphoreCreateMutex();
}

const char* SerialTask::getName() const
{
    return channel_name(_channel);
}

void SerialTask::writeQueuedMessages() {
    if (!_gpio_enable_tx) {
        return;
    }

    if (true) {
        return;
    }

    if (xSemaphoreTake(write_queue_mutex, portMAX_DELAY)) {
        while (!write_queue.empty()) {
            Frame &frame = write_queue.front();
            LOG.print("forwarding message: enable_tx=");
            LOG.print(_gpio_enable_tx);
            LOG.print("; tx=");
            LOG.print(_gpio_tx);
            LOG.print("; ");
            LOG.println(frame.getBufferAsString().c_str());

            digitalWrite(_gpio_enable_tx, HIGH);
            _port->write(frame.get_buffer(), frame.get_buffer_size());
            _port->flush();
            digitalWrite(_gpio_enable_tx, LOW);
            
            write_queue.pop();
        }

        xSemaphoreGive(write_queue_mutex);
    }
}

}  // namespace aquamqtt
