

#include <cassert>
#include <iostream>

#include "encode.hh"
#include "fragmentPipe.hh"
#include "packet.hh"

using namespace MediaNet;

FragmentPipe::FragmentPipe(PipeInterface *t) : PipeInterface(t) {}

bool FragmentPipe::send(std::unique_ptr<Packet> packet) {
  // TODO  break packets larger than 1200 bytes into ecual size framgments less
  // than 1200 bytes

  assert(downStream);
  return downStream->send(move(packet));
}

std::unique_ptr<Packet> FragmentPipe::recv() {

  // TODO - cache fragments and reassemble  and pass on when full packet
  // received

  assert(downStream);
  return downStream->recv();
}
