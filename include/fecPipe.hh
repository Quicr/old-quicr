#pragma once

#include <cstdint>
#include <memory>
#include <list>
#include <utility> // for pair

#include "packet.hh"
#include "pipeInterface.hh"

namespace MediaNet {

class FecPipe : public PipeInterface {
public:
  FecPipe(PipeInterface *t);
  ~FecPipe();

  virtual bool send( std::unique_ptr<Packet> packet);

  /// non blocking, return nullptr if no buffer
  virtual std::unique_ptr<Packet> recv();

private:
    // This is a list of FEC packets to send and time in ms to send them
    std::list<  std::pair< uint32_t /*sendTime*/, std::unique_ptr<Packet> > > sendList;
};

} // namespace MediaNet
