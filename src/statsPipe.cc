

#include <cassert>
#include <iostream>

#include "encode.hh"
#include "packet.hh"
#include "statsPipe.hh"

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
    this->updateRTT(minRtt, bigRtt);
  }
}

void StatsPipe::ack(ShortName name) {
  // this is here just to eat the ACKs and stop propagation up chain
  (void)name;
}
