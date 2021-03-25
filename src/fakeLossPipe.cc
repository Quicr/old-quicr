

#include <cassert>
#include <iostream>

#include "quicr/fakeLossPipe.hh"
#include "quicr/packet.hh"

using namespace MediaNet;

FakeLossPipe::FakeLossPipe(PipeInterface *t)
    : PipeInterface(t), upstreamCount(0), downStreamCount(0) {}

bool FakeLossPipe::send(std::unique_ptr<Packet> packet) {
  assert(nextPipe);

  upstreamCount++;
  if (upstreamCount % 11 == 5) {
    // return true; // TODO remove
  }

  return nextPipe->send(move(packet));
}

std::unique_ptr<Packet> FakeLossPipe::recv() {
  assert(nextPipe);

  auto packet = nextPipe->recv();

  if (packet) {
    downStreamCount++;
    if (downStreamCount % 13 == 7) {
      // return std::unique_ptr<Packet>(nullptr); // TODO remove
    }
  }

  return packet;
}
