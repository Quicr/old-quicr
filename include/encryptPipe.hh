#pragma once

#include <cstdint>
#include <memory>

#include "packet.hh"
#include "pipeInterface.hh"

namespace MediaNet {

class EncryptPipe : public PipeInterface {
public:
  explicit EncryptPipe(PipeInterface *t);

  bool send(std::unique_ptr<Packet> packet) override;

  /// non blocking, return nullptr if no buffer
  std::unique_ptr<Packet> recv() override;

private:
};

} // namespace MediaNet
