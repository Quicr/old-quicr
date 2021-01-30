#pragma once

#include <cstdint>
#include <memory>

#include "packet.hh"
#include "pipeInterface.hh"

namespace MediaNet {

class CrazyBitPipe : public PipeInterface {
public:
  CrazyBitPipe(PipeInterface *t);

  virtual bool send(std::unique_ptr<Packet> packet);

  /// non blocking, return nullptr if no buffer
  virtual std::unique_ptr<Packet> recv();

  virtual void
  updateRTT(uint16_t minRttMs,
            uint16_t maxRttMs); // tells downstream things the current RTT

private:
  uint16_t rttMs;
  bool spinBitVal;
  uint32_t lastSpinTimeMs;
};

} // namespace MediaNet
