#include <cstring>

#include "frame.h"
#include "constants.h"

namespace aquamqtt {

Frame::Frame(FrameChannel channel, uint8_t *_buffer, uint8_t _buffer_size)
    : mChannel(channel)
    , buffer_size(_buffer_size <= MAX_FRAME_SIZE ? _buffer_size : 0)
{
    memcpy(buffer, _buffer, _buffer_size);
}

Frame::Frame(const Frame& other)
    : mChannel(other.mChannel)
    , buffer_size(other.buffer_size <= MAX_FRAME_SIZE ? other.buffer_size : 0)
{
    memcpy(buffer, other.buffer, other.buffer_size);
}

const char* Frame::getChannelName() const {
    return channel_name(mChannel);
}

const uint8_t* Frame::get_buffer() const {
    return buffer;
}

int Frame::get_buffer_size() const {
    return buffer_size;
}

const uint8_t* Frame::payload() const {
    return &buffer[HEADER_LENGTH + 1];
}

int Frame::payload_size() const {
    if (buffer_size == HEADER_LENGTH + 2) {
        return 0;
    } else {
        return buffer_size - 2 - 1 - HEADER_LENGTH;
    }
}

uint64_t Frame::getHeaderValue() const
{
    uint64_t value = 0;
    for (int i=0 ; i<HEADER_LENGTH ; i++) {
        value = value << 8 | buffer[i];
    }
    return value;
}

const std::string Frame::getBufferAsString() const {
    char hex[MAX_FRAME_SIZE * 2 + 1];
    for (int i=0 ; i<buffer_size ; i++) {
        sprintf(hex + 2*i, "%02X", buffer[i]);
    }
    std::string result = hex;
    return result;
}

Frame& Frame::operator=(const Frame& other) {
    if (this == &other) {
        return *this;
    }

    mChannel = other.mChannel;
    buffer_size = other.buffer_size;
    memcpy(buffer, other.buffer, other.buffer_size);

    return *this;
}

bool Frame::operator==(const Frame& other) const {
    if (mChannel != other.mChannel || buffer_size != other.buffer_size) {
        return false;
    }
    if (memcmp(buffer, other.buffer, buffer_size)) {
        return false;
    }
    return true;
}

bool Frame::operator!=(const Frame& other) const {
    return !(*this == other);
}

}
