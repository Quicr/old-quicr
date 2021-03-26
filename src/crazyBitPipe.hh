#pragma once

#include <cstdint>
#include <memory>

#include "pipeInterface.hh"
#include "quicr/packet.hh"

namespace MediaNet {

class CrazyBitPipe : public PipeInterface {
public:
  explicit CrazyBitPipe(PipeInterface *t);

  bool send(std::unique_ptr<Packet> packet) override;

  /// non blocking, return nullptr if no buffer
  std::unique_ptr<Packet> recv() override;

  void updateRTT(uint16_t minRttMs, uint16_t maxRttMs)
      override; // tells downstream things the current RTT

private:
  uint16_t rttMs;
  bool spinBitVal;
  uint32_t lastSpinTimeMs;
};

} // namespace MediaNet
