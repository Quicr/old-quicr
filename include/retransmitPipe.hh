#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <mutex>

#include "packet.hh"
#include "pipeInterface.hh"

namespace MediaNet {

class RetransmitPipe : public PipeInterface {
public:
  RetransmitPipe(PipeInterface *t);

  virtual bool send(std::unique_ptr<Packet> packet);

  /// non blocking, return nullptr if no buffer
  virtual std::unique_ptr<Packet> recv();

  virtual void ack(ShortName name);

  virtual void
  updateRTT(uint16_t minRttMs,
            uint16_t maxRttMs); // tells downstream things the current RTT

private:
  std::mutex rtxListMutex;
  std::map<MediaNet::ShortName, std::unique_ptr<Packet>> rtxList;

  uint32_t maxActTime;

  uint16_t minRtt;
  uint16_t bigRtt;
};

} // namespace MediaNet
