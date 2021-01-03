#pragma once

#include <cstdint>
#include <memory>

#include "packet.hh"
#include "pipeInterface.hh"

namespace MediaNet {

class PriorityPipe : public PipeInterface {
public:
  PriorityPipe(PipeInterface *t);

  virtual bool send(std::unique_ptr<Packet> packet);

private:
};

} // namespace MediaNet
