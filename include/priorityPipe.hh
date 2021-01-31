#pragma once

#include <cstdint>
#include <memory>

#include "packet.hh"
#include "pipeInterface.hh"

namespace MediaNet {

class PriorityPipe : public PipeInterface {
public:
  explicit PriorityPipe(PipeInterface *t);

  bool send(std::unique_ptr<Packet> packet) override;

private:
};

} // namespace MediaNet
