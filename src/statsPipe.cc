

#include <cassert>
#include <iostream>

#include "encode.hh"
#include "statsPipe.hh"
#include "packet.hh"

using namespace MediaNet;

StatsPipe::StatsPipe(PipeInterface *t) : PipeInterface(t) {
    for ( int stat = (int)PipeInterface::StatName::none; stat <= (int)PipeInterface::StatName::bad; stat++ ) {
        stats.insert(std::pair<PipeInterface::StatName, uint64_t>( (PipeInterface::StatName)stat, 0) );
    }
}

void StatsPipe::updateStat(PipeInterface::StatName stat, uint64_t value) {
    stats.at( stat ) = value;
}

void StatsPipe::ack(Packet::ShortName name) {
    (void)name;
}
