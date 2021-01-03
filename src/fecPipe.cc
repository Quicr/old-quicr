

#include <cassert>
#include <iostream>

#include "encode.hh"
#include "fecPipe.hh"
#include "packet.hh"

using namespace MediaNet;

FecPipe::FecPipe(PipeInterface *t) : PipeInterface(t) {}

bool FecPipe::send(std::unique_ptr<Packet> packet) {

  // TODO for packets with FEC enabled, save them and send them again in 10 ms

  assert(downStream);
  return downStream->send(move(packet));
}

std::unique_ptr<Packet> FecPipe::recv() {

  assert(downStream);
  return downStream->recv();
}
