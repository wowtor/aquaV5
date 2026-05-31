#ifndef AQUAMQTT_PROTOCOL_H
#define AQUAMQTT_PROTOCOL_H

#include "frame.h"


namespace aquamqtt{

void process_frame(const Frame &frame);

}
#endif // AQUAMQTT_PROTOCOL_H
