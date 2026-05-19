#ifndef AQUAMQTT_WIFI_H
#define AQUAMQTT_WIFI_H

#include <WiFi.h>

namespace aquamqtt
{

class WifiHandler final
{
public:
    static WifiHandler& getInstance();

    void setup();
    void loop();

    const std::string getIPAddress();

    bool connected();

private:
    WifiHandler();
    ~WifiHandler() = default;

    static void wifiCallback(WiFiEvent_t event);

    unsigned long mLastCheck;

    static bool mConnectedToWifiWithValidIpAddress;

};

}  // namespace aquamqtt



#endif  // AQUAMQTT_WIFI_H
