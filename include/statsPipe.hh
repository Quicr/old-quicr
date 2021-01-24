#pragma once

#include <cstdint>
#include <memory>

#include "packet.hh"
#include "pipeInterface.hh"

namespace MediaNet {

class StatsPipe : public PipeInterface {
public:
  StatsPipe(PipeInterface *t);

    virtual void updateStat( StatName stat, uint64_t value  ); // tells upstream things the stat
    virtual void ack( Packet::ShortName name );

private:
};

} // namespace MediaNet
