#pragma once

#include <cstdint>
#include <memory>

#include "packet.hh"
#include "pipeInterface.hh"

#include <sframe/sframe.h>

namespace MediaNet {

class EncryptPipe : public PipeInterface {
public:
  explicit EncryptPipe(PipeInterface *t);

  bool send(std::unique_ptr<Packet> packet) override;

  /// non blocking, return nullptr if no buffer
  std::unique_ptr<Packet> recv() override;

private:
	sframe::MLSContext::SenderID senderId; // eventually comes from MLS
	sframe::MLSContext mls_context;

};

} // namespace MediaNet
