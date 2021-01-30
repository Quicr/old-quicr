#pragma once

#include <memory>
#include <string>

#include "packet.hh"

namespace MediaNet {

class PipeInterface {
public:
    enum struct StatName : uint16_t {
        none = 0, // must be first
        mtu ,
        minRTTms ,
        bigRTTms ,
        bad // must be last
    };

  virtual bool start(const uint16_t port, const std::string server,
                     PipeInterface *upStream);
  virtual bool ready() const;
  virtual void stop();

  virtual bool send(std::unique_ptr<Packet>);

  /// non blocking, return nullptr if no buffer
  virtual std::unique_ptr<Packet> recv();

  virtual bool fromDownstream(std::unique_ptr<Packet>);

  virtual void updateStat( StatName stat, uint64_t value  ); // tells upstream things the stat
  virtual void ack( Packet::ShortName name ); // tells upstream things name was received
  virtual void updateRTT( uint16_t minRttMs, uint16_t bitRttMs  ); // tells downstream things the current RTT

protected:
  PipeInterface(PipeInterface *downStream);
  virtual ~PipeInterface();

  PipeInterface *downStream;
  PipeInterface *upStream;
};

} // namespace MediaNet
