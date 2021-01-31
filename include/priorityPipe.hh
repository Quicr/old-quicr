#pragma once

#include <cstdint>
#include <memory>
#include <mutex>
#include <queue>

#include "packet.hh"
#include "pipeInterface.hh"

namespace MediaNet {

class PriorityPipe : public PipeInterface {
public:
  explicit PriorityPipe(PipeInterface *t);

  bool send(std::unique_ptr<Packet> packet) override;

  std::unique_ptr<Packet> toDownstream() override;

private:
    std::mutex sendQMutex;
    std::queue<std::unique_ptr<Packet>> sendQ;


};

} // namespace MediaNet
