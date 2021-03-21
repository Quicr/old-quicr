#pragma once

#include <cstdint>
#include <memory>

#include "pipeInterface.hh"
#include "quicr/packet.hh"

#include <sframe/sframe.h>

namespace MediaNet {

class EncryptPipe : public PipeInterface {
public:
  explicit EncryptPipe(PipeInterface *t);

  bool send(std::unique_ptr<Packet> packet) override;

  /// non blocking, return nullptr if no buffer
  std::unique_ptr<Packet> recv() override;

  // initialize sframe context for the given epoch and secret
  // Note: epocha and epoch_secret comes from MLS group context.
  void setCryptoKey(sframe::MLSContext::EpochID epoch,
                    const sframe::bytes &mls_epoch_secret);

private:
  sframe::bytes protect(const std::unique_ptr<Packet> &packet,
                        uint16_t payloadSize);
  sframe::bytes unprotect(const std::unique_ptr<Packet> &packet,
                          uint16_t payloadSize);

  sframe::MLSContext::EpochID current_epoch = -1;
  sframe::MLSContext mls_context;
};

} // namespace MediaNet
