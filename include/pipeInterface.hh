#pragma once

#include <memory>
#include <string>

#include "packet.hh"

namespace MediaNet {

class PipeInterface {
public:
    enum struct StatName : uint16_t {
        bad = 0,
        mtu = 1,
        minRTTms = 2,
        bigRTTms = 3,
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
  virtual void updateRTT( uint64_t rttMs ); // tells downstream things the current RTT

protected:
  PipeInterface(PipeInterface *downStream);
  virtual ~PipeInterface();

  PipeInterface *downStream;
  PipeInterface *upStream;
};

} // namespace MediaNet
