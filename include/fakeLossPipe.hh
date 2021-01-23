#pragma once

#include <cstdint>
#include <memory>

#include "packet.hh"
#include "pipeInterface.hh"

namespace MediaNet {

class FakeLossPipe : public PipeInterface {
public:
  FakeLossPipe(PipeInterface *t);

  virtual bool send(std::unique_ptr<Packet> packet);

  /// non blocking, return nullptr if no buffer
  virtual std::unique_ptr<Packet> recv();

private:
  int upstreamCount;
  int downStreamCount;
};

} // namespace MediaNet
