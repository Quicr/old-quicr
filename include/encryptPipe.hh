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

	void setCryptoKey(sframe::MLSContext::EpochID epoch,
									 const sframe::bytes &mls_epoch_secret);


private:

	void protect(const std::unique_ptr<Packet>& packet);
	void unprotect(const std::unique_ptr<Packet>& packet);

	sframe::MLSContext::EpochID current_epoch = -1;
	sframe::MLSContext mls_context;
};

} // namespace MediaNet
