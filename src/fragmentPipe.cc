

#include <cassert>
#include <iostream>

#include "encode.hh"
#include "fragmentPipe.hh"
#include "packet.hh"

using namespace MediaNet;

FragmentPipe::FragmentPipe(PipeInterface *t) : PipeInterface(t),mtu(1200) {}


void FragmentPipe::updateStat(PipeInterface::StatName stat, uint64_t value) {
    if ( stat == PipeInterface::StatName::mtu ) {
        mtu = (int)value;
    }

    PipeInterface::updateStat(stat, value);
}


bool FragmentPipe::send(std::unique_ptr<Packet> packet) {
    // TODO  break packets larger than mtu bytes into equal size fragments less
    mtu = 100; // TODO REMOVE - just for testing

    if (nextTag(packet) == PacketTag::appData) {
        std::clog << "Packet fullSize=" << packet->fullSize() << " size=" << packet->size() << std::endl;
    }
    else {
        assert(0);
    }

    assert(downStream);
    return downStream->send(move(packet));
}

std::unique_ptr<Packet> FragmentPipe::recv() {
    // TODO - cache fragments and reassemble then pass on the when full packet is received

  assert(downStream);
  return downStream->recv();
}
