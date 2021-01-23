

#include <cassert>
#include <iostream>

#include "encode.hh"
#include "encryptPipe.hh"
#include "packet.hh"

using namespace MediaNet;

EncryptPipe::EncryptPipe(PipeInterface *t) : PipeInterface(t) {}

bool EncryptPipe::send(std::unique_ptr<Packet> packet) {

  // TODO encrypt packet and add auth tag
  // The message at this point has the shortName followed by the data in the
  // buffer use the shortname to lookup key for sender and form the IV encrypt
  // with this key and add auth data

  assert(downStream);
  return downStream->send(move(packet));
}

std::unique_ptr<Packet> EncryptPipe::recv() {

  // TODO check packet integrity and decrypt

  assert(downStream);
  return downStream->recv();
}
