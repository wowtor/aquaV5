#include <memory>
#include <string>
#include <stdexcept>

#include "constants.h"


namespace aquamqtt {


const char* channel_name(FrameChannel channel) {
    switch(channel) {
    case FrameChannel::CH_HMI:
        return HMI_TASK_NAME;
    case FrameChannel::CH_MAIN:
        return CONTROLLER_TASK_NAME;
    case FrameChannel::CH_LISTENER:
        return LISTENER_TASK_NAME;
    default:
        return "unknown";
    }
}


char *device_id = nullptr;
const char* unique_device_id() {
    if (!device_id) {
        uint64_t chipid;
        chipid = ESP.getEfuseMac();
        size_t n = snprintf(nullptr, 0, "%08x", (uint32_t)chipid) + 1;
        device_id = new char[n];
        snprintf(device_id, n, "%08x", (uint32_t)chipid);
    }
    return device_id;
}

}
