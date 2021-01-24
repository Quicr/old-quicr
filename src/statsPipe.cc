

#include <cassert>
#include <iostream>

#include "encode.hh"
#include "statsPipe.hh"
#include "packet.hh"

using namespace MediaNet;

StatsPipe::StatsPipe(PipeInterface *t) : PipeInterface(t) {}

void StatsPipe::updateStat(PipeInterface::StatName stat, uint64_t value) {
    // TODO
}

void StatsPipe::ack(Packet::ShortName name) {
    (void)name;
}
