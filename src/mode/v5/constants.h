#ifndef AQUAMQTT_CONSTANTS_H
#define AQUAMQTT_CONSTANTS_H

#include <Arduino.h>
#include "message/MessageConstants.h"
#include "config/Configuration.h"

namespace aquamqtt
{

#define MQTT_PUBLISH_FRAMES

#define LOG Serial

#define APP_NAME "AquaV5"

#define DEVICE_NAME "DHW"

#define HMI_TASK_NAME "hmi"
#define CONTROLLER_TASK_NAME "controller"
#define LISTENER_TASK_NAME "listener"

#define FrameChannel message::FrameBufferChannel

}  // namespace aquamqtt

#endif  // AQUAMQTT_CONSTANTS_H
