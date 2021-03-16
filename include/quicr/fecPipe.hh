#pragma once

#include <cstdint>
#include <list>
#include <memory>
#include <utility> // for pair

#include "packet.hh"
#include "pipeInterface.hh"

namespace MediaNet {

class FecPipe : public PipeInterface {
public:
  explicit FecPipe(PipeInterface *t);
  ~FecPipe() override;

  bool send(std::unique_ptr<Packet> packet) override;

  /// non blocking, return nullptr if no buffer
  std::unique_ptr<Packet> recv() override;

private:
  // This is a list of FEC packets to send and time in ms to send them
  std::list<std::pair<uint32_t /*sendTime*/, std::unique_ptr<Packet>>> sendList;
};

} // namespace MediaNet
