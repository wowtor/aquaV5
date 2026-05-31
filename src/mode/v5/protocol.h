#ifndef AQUAMQTT_PROTOCOL_H
#define AQUAMQTT_PROTOCOL_H

#include "frame.h"


namespace aquamqtt{

bool check_crc(const uint8_t* frame, const uint8_t len);

void process_frame(const Frame &frame);

}
#endif // AQUAMQTT_PROTOCOL_H
