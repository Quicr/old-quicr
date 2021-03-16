#pragma once

#include <array>
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
  std::unique_ptr<Packet> recv() override;

  std::unique_ptr<Packet> toDownstream() override;
  bool fromDownstream(std::unique_ptr<Packet>) override;

  void updateMTU(uint16_t mtu, uint32_t pps) override;

private:
  static const int maxPriority = 10;

  std::mutex sendQMutex;
  std::array<std::queue<std::unique_ptr<Packet>>, maxPriority + 1> sendQarray;

  std::queue<std::unique_ptr<Packet>> recvQ;
  std::mutex recvQMutex;

  uint16_t mtu;
};

} // namespace MediaNet
