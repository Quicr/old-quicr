

#include <cassert>
#include <iostream>

#include "packet.hh"
#include "fakeLossPipe.hh"

using namespace MediaNet;

FakeLossPipe::FakeLossPipe(PipeInterface *t) : PipeInterface(t) , upstreamCount(0), downStreamCount(0)
{}

bool FakeLossPipe::send(std::unique_ptr<Packet> packet) {
  assert(downStream);

    upstreamCount++;
  if ( upstreamCount % 11 == 5 ) {
      return true;
  }

  return downStream->send(move(packet));
}

std::unique_ptr<Packet> FakeLossPipe::recv() {
  assert(downStream);

  auto packet = downStream->recv();

    if ( packet ) {
        downStreamCount++;
        if (downStreamCount % 13 == 7) {
            return std::unique_ptr<Packet>(nullptr);
        }
    }

  return packet;
}
