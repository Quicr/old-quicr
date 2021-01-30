#pragma once

#include <cstdint>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

#include "packet.hh"
#include "pipeInterface.hh"

namespace MediaNet {

class ConnectionPipe : public PipeInterface {
public:
  explicit ConnectionPipe(PipeInterface *t);
  void setAuthInfo(uint32_t sender, uint64_t t);

  bool start(uint16_t port, std::string server,
                     PipeInterface *upStream) override;
  [[nodiscard]] bool ready() const override;
  void stop() override;

  std::unique_ptr<Packet> recv() override;

private:
  uint32_t senderID;
  uint64_t token;

  bool open;
};

} // namespace MediaNet