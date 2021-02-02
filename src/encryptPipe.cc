

#include <cassert>
#include <iostream>

#include "encryptPipe.hh"
#include "packet.hh"

using namespace MediaNet;

// TODO fix this when we support crypto agility
static const auto FIXED_CIPHER_SUITE = sframe::CipherSuite::AES_GCM_128_SHA256;
static const size_t SFRAME_EPOCH_BITS = 8;


EncryptPipe::EncryptPipe(PipeInterface *t):
	PipeInterface(t),
	mls_context(FIXED_CIPHER_SUITE, SFRAME_EPOCH_BITS){}

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
