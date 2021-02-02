

#include <cassert>
#include <iostream>

#include "encryptPipe.hh"
#include "packet.hh"

using namespace MediaNet;

// TODO fix this when we support crypto agility
static const auto FIXED_CIPHER_SUITE = sframe::CipherSuite::AES_GCM_128_SHA256;
static const size_t SFRAME_EPOCH_BITS = 8;

template<typename T>
sframe::bytes
static to_bytes(const T& range)
{
	return sframe::bytes(range.begin(), range.end());
}

EncryptPipe::EncryptPipe(PipeInterface *t):
	PipeInterface(t),
	mls_context(FIXED_CIPHER_SUITE, SFRAME_EPOCH_BITS){}

bool EncryptPipe::send(std::unique_ptr<Packet> packet) {

  // TODO encrypt packet and add auth tag
  // The message at this point has the shortName followed by the data in the
  // buffer use the shortname to lookup key for sender and form the IV encrypt
  // with this key and add auth data

  assert(downStream);
  protect(packet);
  return downStream->send(move(packet));
}

std::unique_ptr<Packet> EncryptPipe::recv() {

  // TODO check packet integrity and decrypt

  assert(downStream);
  auto packet = downStream->recv();
  unprotect(packet);
  return packet;
}

void EncryptPipe::setCryptoKey(sframe::MLSContext::EpochID epoch,
															 const sframe::bytes &mls_epoch_secret) {
	current_epoch = epoch;
	mls_context.add_epoch(epoch, std::move(mls_epoch_secret));
}


void EncryptPipe::protect(const std::unique_ptr<Packet>& packet)
{
	assert(current_epoch != -1);
	gsl::span<const uint8_t> buffer_ref = packet->buffer;
	auto plaintext = buffer_ref.subspan(packet->headerSize, (packet->size() - packet->headerSize));
	auto ct_out = sframe::bytes(plaintext.size() + sframe::max_overhead);
	auto ct = mls_context.protect(current_epoch, packet->name.senderID, ct_out, plaintext);
	packet->buffer = to_bytes(ct);
}

void EncryptPipe::unprotect(const std::unique_ptr<Packet>& packet)
{
	gsl::span<uint8_t> buffer_ref = packet->buffer;
	// start decryption from data (excluding header)
	auto ciphertext = buffer_ref.subspan(packet->headerSize, (packet->size() - packet->headerSize));
	auto pt_out = sframe::bytes(ciphertext.size());
	std::cout << "Packet Size: " << packet->size() << "\n";
	std::cout << "CipherText Size: " << ciphertext.size() << "\n";
	auto pt = mls_context.unprotect(pt_out, ciphertext);
	std::cout << "DecryptedText Size: " << pt.size() << "\n";
	packet->buffer = to_bytes(pt);
}