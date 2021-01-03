#pragma once

#include <memory>
#include <string>

#include "packet.hh"

namespace MediaNet {

class PipeInterface {
public:
  virtual bool start(const uint16_t port, const std::string server,
                     PipeInterface *upStream);
  virtual bool ready() const;
  virtual void stop();

  virtual bool send(std::unique_ptr<Packet>);

  /// non blocking, return nullptr if no buffer
  virtual std::unique_ptr<Packet> recv();

  virtual bool fromDownstream(std::unique_ptr<Packet>);

protected:
  PipeInterface(PipeInterface *downStream);
  virtual ~PipeInterface();

  PipeInterface *downStream;
  PipeInterface *upStream;
};

} // namespace MediaNet
