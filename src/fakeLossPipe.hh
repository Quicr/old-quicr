#pragma once

#include <cstdint>
#include <memory>

#include "pipeInterface.hh"
#include "quicr/packet.hh"

namespace MediaNet {

class FakeLossPipe : public PipeInterface {
public:
  explicit FakeLossPipe(PipeInterface *t);

  bool send(std::unique_ptr<Packet> packet) override;

  /// non blocking, return nullptr if no buffer
  std::unique_ptr<Packet> recv() override;

private:
  int upstreamCount;
  int downStreamCount;
};

} // namespace MediaNet
