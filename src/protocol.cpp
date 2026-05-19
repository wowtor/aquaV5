#include "protocol.h"

#include "dhwstate.h"


namespace aquamqtt {

float parse_temperature(const uint8_t* bytes)
{
    int16_t value = bytes[0] << 8 | bytes[1];
    return (float)value / 100;
}

bool check_payload_size(const Frame &frame, int expected_size, const char* frame_type_name) {
    if (frame.payload_size() == 0) {
        return false; // no payload
    }

    if (frame.payload_size() != expected_size) {
        LOG.print("[");
        LOG.print(frame.getChannelName());
        LOG.print("] invalid frame: ");
        LOG.print(frame_type_name);
        LOG.print(" should have payload size ");
        LOG.print(expected_size);
        LOG.print("; found: ");
        LOG.println(frame.payload_size());
        return false;
    }

    return true;
}

void process_temperature_frame(DhwState &state, const Frame &frame) {
    if (!check_payload_size(frame, 12, "temperature")) {
        return;
    }

    const uint8_t* payload = frame.payload();
    state.water_temperature->set_value(parse_temperature(&payload[0]));
    state.compressor_outlet_temperature->set_value(parse_temperature(&payload[2]));
    state.air_inlet_temperature->set_value(parse_temperature(&payload[4]));
    state.evaporator1_temperature->set_value(parse_temperature(&payload[6]));
    state.evaporator2_temperature->set_value(parse_temperature(&payload[8]));
    state.evaporator3_temperature->set_value(parse_temperature(&payload[10]));
}

void process_input_frame(DhwState &state, const Frame &frame) {
    if (!check_payload_size(frame, 3, "input")) {
        return;
    }

    state.input_i2->set_value(frame.payload()[0]);
    state.input_i1->set_value(frame.payload()[1]);
    state.heating_active->set_value(frame.payload()[2]);
}

void process_frame(const Frame &frame)
{
    DhwState &state = DhwState::getInstance();

    uint64_t header = frame.getHeaderValue();

    switch(header) {
    case 0x0164FEB006:
        process_temperature_frame(state, frame);
        break;
    case 0x0164FF1403:
        process_input_frame(state, frame);
        break;
    }
}

} // namespace aquamqtt
