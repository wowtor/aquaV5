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

}
