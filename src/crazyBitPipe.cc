

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
  // TODO - clear the spin bit in first byte of incoming packet

  assert(downStream);
  return downStream->recv();
}
