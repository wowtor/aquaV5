#ifndef MESSAGEBUFFER_H
#define MESSAGEBUFFER_H

#include <FastCRC.h>

#include "constants.h"
#include "frame.h"
#include "config.h"

namespace aquamqtt{

class FrameBuffer final
{
private:
  FrameChannel channel;
  uint64_t n_frames_received = 0;
  uint64_t n_err_checksum = 0;
  uint64_t n_err_frame_too_long = 0;
  uint64_t n_err_buffer_full = 0;
  uint64_t n_err_no_magic = 0;

  uint8_t buffer[MAX_FRAME_SIZE];
  uint8_t buffer_size = 0;

  uint8_t dropped[MAX_FRAME_SIZE];
  uint8_t dropped_size = 0;

  FastCRC16 mCRC;

  bool checkCRC();
  Frame *handleFrame();

public:
  FrameBuffer(FrameChannel channel);
  ~FrameBuffer() = default;

  inline uint64_t countFramesReceived() const { return n_frames_received; };
  inline uint64_t countChecksumErrors() const { return n_err_checksum; };
  inline uint64_t countLongFrameErrors() const { return n_err_frame_too_long; };
  inline uint64_t countBufferFullErrors() const { return n_err_buffer_full; };
  inline uint64_t countNoMagicErrors() const { return n_err_no_magic; };

  void pushByte(uint8_t val);
};

}
#endif //MESSAGEBUFFER_H
