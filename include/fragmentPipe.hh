#pragma once

#include <cstdint>
#include <memory>

#include "packet.hh"
#include "pipeInterface.hh"

namespace MediaNet {

class FragmentPipe : public PipeInterface {
public:
  explicit FragmentPipe(PipeInterface *t);

  virtual bool send(std::unique_ptr<Packet> packet);

  virtual void updateStat( StatName stat, uint64_t value  );

  /// non blocking, return nullptr if no buffer
  virtual std::unique_ptr<Packet> recv();

private:
    uint16_t mtu;
};

} // namespace MediaNet
