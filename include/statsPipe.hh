#pragma once

#include <cstdint>
#include <map>
#include <memory>

#include "packet.hh"
#include "pipeInterface.hh"

namespace MediaNet {

class StatsPipe : public PipeInterface {
public:
  explicit StatsPipe(PipeInterface *t);

  void updateStat(StatName stat,
                  uint64_t value) override; // tells upstream things the stat

    void updateRTT(uint16_t minRttMs, uint16_t bitRttMs) override;

    void updateMTU(uint16_t mtu, uint32_t pps) override;

private:
  std::map<PipeInterface::StatName, uint64_t> stats;
};

} // namespace MediaNet
