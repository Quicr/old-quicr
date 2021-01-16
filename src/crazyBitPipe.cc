

#include <cassert>
#include <iostream>

#include "crazyBitPipe.hh"
#include "encode.hh"
#include "packet.hh"

using namespace MediaNet;

CrazyBitPipe::CrazyBitPipe(PipeInterface *t) : PipeInterface(t) {}

bool CrazyBitPipe::send(std::unique_ptr<Packet> packet) {
  // TODO - set the spin bit in first byte of outgoing packet

  assert(downStream);
  return downStream->send(move(packet));
}

std::unique_ptr<Packet> CrazyBitPipe::recv() {
    assert(downStream);
    std::unique_ptr<Packet> packet = downStream->recv();
    if ( !packet ) {
        return packet;
    }

    // clear the spin bit in first byte of incoming packet
    assert( packet->buffer.size() >= 1 );
    packet->buffer[0] &= 0xFE;

    return packet;
}
