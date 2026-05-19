#include "framebuffer.h"

#include "serialtask.h"
#include "mqtttask.h"
#include "protocol.h"

namespace aquamqtt {

/*** NON-MEMBER FUNCTIONS ***/

void frameReceived(const Frame &frame, bool dropped) {
    /*
    LOG.print("[framebuffer] processing message: header=");
    LOG.print(message.getHeaderValue());
    LOG.print("; payload_size=");
    LOG.print(message.payload_size());
    LOG.println();
    */

    // forward frame
    switch (frame.getChannel()) {
    case FrameChannel::CH_HMI:
        //LOG.println("forwarding frame to controller");
        SerialTask::getControllerInstance().queueSendFrame(frame);
        break;
    case FrameChannel::CH_MAIN:
        //LOG.println("forwarding frame to hmi");
        SerialTask::getHMIInstance().queueSendFrame(frame);
        break;
    }

    // send frame to MQTT for debugging
    #ifdef MQTT_PUBLISH_FRAMES
    if (dropped) {
        MqttTask::getInstance().queueDroppedBytes(frame);
    } else {
        MqttTask::getInstance().queueFrame(frame);
        process_frame(frame);
    }
    #endif
}

/*** FRAMEBUFFER PUBLIC ***/

FrameBuffer::FrameBuffer(FrameChannel _channel)
    : channel(_channel)
{
}

void FrameBuffer::pushByte(const uint8_t val)
{
    switch (channel) {
    case FrameChannel::CH_HMI:
        //LOG.println("forwarding frame to controller");
        SerialTask::getControllerInstance().sendByte(val);
        break;
    case FrameChannel::CH_MAIN:
        //LOG.println("forwarding frame to hmi");
        SerialTask::getHMIInstance().sendByte(val);
        break;
    }

    // drop old bytes if the buffer is full
    if (buffer_size >= MAX_FRAME_SIZE) {
        frameReceived(Frame(channel, buffer, buffer_size), true);
        n_err_buffer_full++;
        buffer_size = 0;
    }

    // append the new element to the buffer
    buffer[buffer_size++] = val;

    // see if the buffer contains a full frame
    Frame* message = handleFrame();
    if (message) {
        frameReceived(*message, false);
        delete message;
    }
}

/*** FRAMEBUFFER PRIVATE ***/

bool FrameBuffer::checkCRC() {
    uint16_t actualCRC  = mCRC.modbus(buffer, buffer_size - 2);
    uint16_t desiredCRC = buffer[buffer_size-1] << 8 | buffer[buffer_size-2];
    return desiredCRC == actualCRC;
}

Frame * FrameBuffer::handleFrame()
{
    if (dropped_size > MAX_FRAME_SIZE - 2) {
        frameReceived(Frame(channel, dropped, dropped_size), true);
        dropped_size = 0;
    }

    // first byte must be 0x01
    if (buffer_size == 1 && buffer[0] != 0x01) {
        dropped[dropped_size++] = buffer[0];
        //LOG.print("dropping first byte: ");
        //LOG.println(buffer[0]);
        n_err_no_magic++;
        buffer_size = 0;
        return nullptr;
    }

    // second byte must be 0x64 or 0x65
    if (buffer_size == 2 && buffer[1] != 0x64 && buffer[1] != 0x65) {
        dropped[dropped_size++] = buffer[0];
        dropped[dropped_size++] = buffer[1];
        //LOG.print("dropping first 2 bytes: ");
        //LOG.print(buffer[0]);
        //LOG.print("-");
        //LOG.println(buffer[1]);
        n_err_no_magic++;
        buffer_size = 0;
        return nullptr;
    }

    if (dropped_size > 0) {
        frameReceived(Frame(channel, dropped, dropped_size), true);
        dropped_size = 0;
    }

    if (buffer_size < HEADER_LENGTH + 2) {
        return nullptr;
    }

    // check if we have a message without a payload
    if (buffer_size == HEADER_LENGTH + 2) {
        if (checkCRC()) {
            // CRC match -> we have a message without a payload
            n_frames_received++;
            Frame* msg = new Frame(channel, buffer, buffer_size);
            buffer_size = 0;
            return msg;
        }
    }

    int payloadLength = buffer[HEADER_LENGTH];
    int messageLength = HEADER_LENGTH + 1 + payloadLength;

    if (messageLength + 2 > MAX_FRAME_SIZE) {
        LOG.println("[framebuffer] message has invalid length");
        n_err_frame_too_long++;
        frameReceived(Frame(channel, buffer, buffer_size), true);
        buffer_size = 0;
        return nullptr;
    }

    // wait until buffer holds a complete frame
    if (buffer_size < messageLength + 2) {
        return nullptr;
    }

    if (!checkCRC()) {
        LOG.println("[framebuffer] invalid CRC");
        n_err_checksum++;
        frameReceived(Frame(channel, buffer, buffer_size), true);
        buffer_size = 0;
        return nullptr;
    }

    // completed and valid frame
    n_frames_received++;
    Frame* msg = new Frame(channel, buffer, buffer_size);
    buffer_size = 0;
    return msg;
}

}
