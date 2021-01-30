#pragma once

#include <cstdint>
#include <map>
#include <memory>

#include "packet.hh"
#include "pipeInterface.hh"

namespace MediaNet {

class StatsPipe : public PipeInterface {
public:
  StatsPipe(PipeInterface *t);

  virtual void updateStat(StatName stat,
                          uint64_t value); // tells upstream things the stat
  virtual void ack(ShortName name);

private:
  std::map<PipeInterface::StatName, uint64_t> stats;
};

} // namespace MediaNet
