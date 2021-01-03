

#include <cassert>
#include <iostream>

#include "encode.hh"
#include "encryptPipe.hh"
#include "packet.hh"

using namespace MediaNet;

EncryptPipe::EncryptPipe(PipeInterface *t) : PipeInterface(t) {}

bool EncryptPipe::send(std::unique_ptr<Packet> packet) {

  // TODO encrypt packet and add auth tag

  assert(downStream);
  return downStream->send(move(packet));
}

std::unique_ptr<Packet> EncryptPipe::recv() {

  // TODO check packet intgretity and decrypt

  assert(downStream);
  return downStream->recv();
}
