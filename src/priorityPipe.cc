

#include <cassert>
#include <iostream>

#include "encode.hh"
#include "packet.hh"
#include "priorityPipe.hh"

using namespace MediaNet;

PriorityPipe::PriorityPipe(PipeInterface *t) : PipeInterface(t) {}

bool PriorityPipe::send(std::unique_ptr<Packet> packet) {
  assert(downStream);

  // TODO - implement priority Q that rate controller can use

  return downStream->send(move(packet));
}
