#ifndef MESSAGE_H
#define MESSAGE_H

#include <cstdint>
#include <string>

#include "constants.h"
#include "util.h"

#define HEADER_LENGTH 5

// longest message is 5 (header) + 17 (payload) + 2 (crc)
#define MAX_FRAME_SIZE 29


namespace aquamqtt{

class Frame;

class Frame final
{
private:
    FrameChannel mChannel;
    uint8_t buffer[MAX_FRAME_SIZE];
    uint8_t buffer_size;

public:
    inline FrameChannel getChannel() const { return mChannel; };
    const char* getChannelName() const;

    const uint8_t* payload() const;
    int payload_size() const;
    uint64_t getHeaderValue() const;

    const uint8_t* get_buffer() const;
    int get_buffer_size() const;

    const std::string getBufferAsString() const;

    Frame& operator=(const Frame& other);
    bool operator==(const Frame& other) const;
    bool operator!=(const Frame& other) const;

public:
    Frame(FrameChannel channel, uint8_t* buffer, uint8_t buffer_size);
    Frame(const Frame& other);
    ~Frame() = default;
};

}
#endif //MESSAGE_H
