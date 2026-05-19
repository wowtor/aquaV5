#ifndef AQUAMQTT_CONSTANTS_H
#define AQUAMQTT_CONSTANTS_H

#include <Arduino.h>

namespace aquamqtt
{

#define MQTT_PUBLISH_FRAMES

#define LOG Serial

#define DEVICE_ID "v5alt"
#define DEVICE_NAME "DHW"

#define HMI_TASK_NAME "hmi"
#define CONTROLLER_TASK_NAME "controller"
#define LISTENER_TASK_NAME "listener"

enum class FrameChannel
{
    CH_LISTENER,
    CH_MAIN,
    CH_HMI
};

enum OperationMode
{
    /**
     *  AquaMqtt is acting as Listener and is connected to a single physical one-wire USART instance:
     * - Reads traffic from the HMI and MAIN Controller
     * - Parses and publishes DHW messages to MQTT
     */
    LISTENER,

    /**
     * AquaMqtt is acting as Man-In-The-Middle and is connected to two physical one-wire USART instances:
     * - Forwards data from the HMI Controller to the MAIN Controller
     * - Forwards data from the MAIN Controller to the HMI Controller
     * - Possibility to overwrite dedicated fields in the messages from HMI to Main Controller
     * - Parses and publishes DHW messages to MQTT, Allows modification via MQTT
     */
    MITM,
};

namespace config {
/**
 * Self explanatory internal settings: most probably you don't want to change them.
 */
constexpr uint32_t WATCHDOG_TIMEOUT_MS   = 60000;
constexpr uint16_t WIFI_RECONNECT_CYCLE_S = 10;
constexpr uint8_t  MQTT_MAX_TOPIC_SIZE   = 80;
constexpr uint8_t  MQTT_MAX_PAYLOAD_SIZE = 255;

/**
 * Default serial configuration uses two stop bits SERIAL_8N2, but there are heatpumps
 * such as the Thermor Aeromax 5 (E/H) #80 which require SERIAL_8N1 here. Else messages
 * are most of the time not complete.
 */
constexpr auto DEFAULT_SERIAL_CONFIGURATION = SERIAL_8N1;

/**
 * Default serial configuration uses baud rate of 9550 / determined using logic analyzer
 */
constexpr unsigned long DEFAULT_SERIAL_BAUD = 38400;

/**
 * Pin assignments for AquaMQTT Board Rev 1.0
 * There are different GPIO ports depending on the hardware.
 * The ENV_DEVKIT_ESP32 macro is set by the platformio environment.
 * By default we use the pin assignments of the Arduino Nano ESP32
 */
#ifdef ENV_DEVKIT_ESP32
constexpr uint8_t GPIO_MAIN_RX = 5;
constexpr uint8_t GPIO_MAIN_TX = 6;
constexpr uint8_t GPIO_MAIN_ENABLE_TX = 9;
constexpr uint8_t GPIO_HMI_RX  = 7;
constexpr uint8_t GPIO_HMI_TX  = 8;
constexpr uint8_t GPIO_HMI_ENABLE_TX = 10;
#else
constexpr uint8_t GPIO_MAIN_RX = 2;
constexpr uint8_t GPIO_MAIN_TX = 3;
constexpr uint8_t GPIO_MAIN_ENABLE_TX = 6;
constexpr uint8_t GPIO_HMI_RX  = 4;
constexpr uint8_t GPIO_HMI_TX  = 5;
constexpr uint8_t GPIO_HMI_ENABLE_TX = 7;
#endif
constexpr uint8_t GPIO_SDA_RTC = A4;
constexpr uint8_t GPIO_SCL_RTC = A5;

/**
 * Parametrize kalman filter for reading temperature values
 * Measurement Uncertainty - How much do we expect to our measurement vary
 */
constexpr float KALMAN_MEA_E = 0.1;

/**
 * Parametrize kalman filter for reading temperature values
 * Estimation Uncertainty - Can be initialized with the same value as e_mea since the kalman filter will adjust its
 * value.
 */
constexpr float KALMAN_EST_E = 0.1;

/**
 * Parametrize kalman filter for reading temperature values
 * Process Variance - usually a small number between 0.001 and 1 - how fast your measurement moves.
 */
constexpr float KALMAN_Q = 0.01;

}  // namespace config
}  // namespace aquamqtt

#endif  // AQUAMQTT_CONSTANTS_H
