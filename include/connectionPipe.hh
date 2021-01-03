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
  ConnectionPipe(PipeInterface *t);
  void setAuthInfo(uint32_t sender, uint64_t t);

  virtual bool start(const uint16_t port, const std::string server,
                     PipeInterface *upStream);
  virtual bool ready() const;
  virtual void stop();

  virtual std::unique_ptr<Packet> recv();

private:
  uint32_t senderID;
  uint64_t token;

  bool open;
};

} // namespace MediaNet