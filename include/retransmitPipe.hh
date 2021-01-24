#pragma once

#include <cstdint>
#include <memory>
#include <map>

#include "packet.hh"
#include "pipeInterface.hh"

namespace MediaNet {

class RetransmitPipe : public PipeInterface {
public:
  RetransmitPipe(PipeInterface *t);

  virtual bool send(std::unique_ptr<Packet> packet);

  /// non blocking, return nullptr if no buffer
  virtual std::unique_ptr<Packet> recv();

  virtual void ack( Packet::ShortName name );

private:

    std::map< Packet::ShortName , std::unique_ptr<Packet> > rtxList;

};

} // namespace MediaNet
