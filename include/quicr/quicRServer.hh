#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "../../src/connectionPipe.hh"
#include "../../src/fakeLossPipe.hh"
#include "../../src/pipeInterface.hh"
#include "../../src/udpPipe.hh"
#include "packet.hh"

namespace MediaNet {

// class UdpPipe;

class QuicRServer {
public:
  QuicRServer();
  virtual ~QuicRServer();

  virtual bool open(uint16_t port);
  virtual bool ready() const;
  virtual void close();
  // Packet might be null
  virtual std::unique_ptr<Packet> recv();
  virtual bool send(std::unique_ptr<Packet>);

private:
  UdpPipe udpPipe;
  FakeLossPipe fakeLossPipe;
  ServerConnectionPipe connectionPipe;
  PipeInterface *firstPipe;
};

} // namespace MediaNet
