#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "connectionPipe.hh"
#include "crazyBitPipe.hh"
#include "encryptPipe.hh"
#include "fakeLossPipe.hh"
#include "fecPipe.hh"
#include "fragmentPipe.hh"
#include "pacerPipe.hh"
#include "packet.hh"
#include "pipeInterface.hh"
#include "priorityPipe.hh"
#include "retransmitPipe.hh"
#include "statsPipe.hh"
#include "subscribePipe.hh"
#include "udpPipe.hh"

namespace MediaNet {

// class UdpPipe;

class QuicRServer {
public:
	QuicRServer();
  virtual ~QuicRServer();

  virtual bool open(uint16_t port);
  virtual bool ready() const;
  virtual void close();
  virtual std::unique_ptr<Packet> recv();
	virtual bool send(std::unique_ptr<Packet>);

private:
  UdpPipe udpPipe;
  FakeLossPipe fakeLossPipe;
  ServerConnectionPipe connectionPipe;
  PipeInterface *firstPipe;

};

} // namespace MediaNet
