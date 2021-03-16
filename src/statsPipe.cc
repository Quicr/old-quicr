

#include <cassert>
#include <iostream>

#include "quicr/statsPipe.hh"

using namespace MediaNet;

StatsPipe::StatsPipe(PipeInterface *t) : PipeInterface(t) {
  for (int stat = (int)PipeInterface::StatName::none;
       stat <= (int)PipeInterface::StatName::bad; stat++) {
    stats.insert(std::pair<PipeInterface::StatName, uint64_t>(
        (PipeInterface::StatName)stat, 0));
  }
}

void StatsPipe::updateStat(PipeInterface::StatName stat, uint64_t value) {
  stats.at(stat) = value;

  // TODO if min or max rtt to a set of all of them
  if (stat == PipeInterface::StatName::minRTTms) {
    uint16_t minRtt = stats.at(PipeInterface::StatName::minRTTms);
    uint16_t bigRtt = stats.at(PipeInterface::StatName::bigRTTms);

    if (minRtt > bigRtt) {
      std::clog << "bigRTT too small mintRtt=" << minRtt
                << "  bigRtt=" << bigRtt << std::endl;
      bigRtt = (minRtt * 3) / 2;
    }
    this->updateRTT(minRtt, bigRtt);
  }

  if (stat == PipeInterface::StatName::mtu) {
    uint16_t mtu = stats.at(PipeInterface::StatName::mtu);
    uint32_t pps = stats.at(PipeInterface::StatName::ppsTargetUp);
    this->updateMTU(mtu, pps);
  }
}

void StatsPipe::updateRTT(uint16_t minRttMs, uint16_t bigRttMs) {
  stats.at(PipeInterface::StatName::minRTTms) = minRttMs;
  stats.at(PipeInterface::StatName::bigRTTms) = bigRttMs;

  PipeInterface::updateRTT(minRttMs, bigRttMs);
}

void StatsPipe::updateMTU(uint16_t mtu, uint32_t pps) {
  stats.at(PipeInterface::StatName::mtu) = mtu;
  stats.at(PipeInterface::StatName::ppsTargetUp) = pps;

  PipeInterface::updateMTU(mtu, pps);
}
