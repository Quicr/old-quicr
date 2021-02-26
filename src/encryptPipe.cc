

#include <cassert>
#include <iostream>
#include <sstream>
#include "encryptPipe.hh"
#include "packet.hh"

using namespace MediaNet;

// TODO Sframe parameters needs to be retrieved from MLS
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

  //TODO: figure out right AAD bits
  assert(downStream);
	 //return downStream->send(std::move(packet));

  auto tag = PacketTag::badTag;
	uint16_t payloadSize = 0;
	ShortName name;
	// strip out [tag, name, payload len]
	packet >> tag;
  assert( tag == PacketTag::appData );
	packet >> name;
	packet >> payloadSize;

	auto encrypted = protect(packet, payloadSize);
	//std::cout << "Payload Original Size/Encrypted Size:" << payloadSize << "/" << encrypted.size() << "\n";

	// re-insert encrypted portion
	uint8_t *dst = &(packet->data());
	packet->resize(encrypted.size() + 22); // 22 - (2) for payloadSize + (19) name + (1) tag
	std::copy(encrypted.begin(), encrypted.end(), dst);
	packet << (uint16_t ) encrypted.size();
	packet << name;
	packet << tag; // TODO - chance to  PacketTag::appDataEncrypted

	//std::cout << "Full Encrypted Packet with header: "<< packet->size() << " bytes\n";

	return downStream->send(move(packet));
}

std::unique_ptr<Packet> EncryptPipe::recv() {

  // TODO check packet integrity and decrypt

  assert(downStream);
  // return downStream->recv();

  auto packet = downStream->recv();
	if(packet) {
		auto tag = nextTag(packet);
		if(tag == PacketTag::pubData) {
			ShortName name;
			uint16_t payloadSize = 0;
			packet >> tag;
			packet >> name;
			packet >> payloadSize;
			assert(payloadSize);

			// TDOO - Cullen look at this seems wrong
			packet->headerSize = packet->fullSize() - payloadSize - 22; // 22 - (2) payloadSize + (19) name +  (1) tag

			auto decrypted = unprotect(packet, payloadSize);
			//std::cout << "Payload Original Size/Decrypted Size:" << payloadSize << "/" << decrypted.size() << "\n";

			packet->resize(decrypted.size() + 22);
			std::copy(decrypted.begin(), decrypted.end(), &packet->data());
			packet << (uint16_t ) decrypted.size();
			packet << name;
			packet << tag;
			//std::cout << "Full Decrypted Packet with header: "<< packet->size() << " bytes\n";
			return packet;
		}
	}
  return packet;
}

void EncryptPipe::setCryptoKey(sframe::MLSContext::EpochID epoch,
															 const sframe::bytes &mls_epoch_secret) {
	current_epoch = epoch;
	mls_context.add_epoch(epoch, std::move(mls_epoch_secret));
	std::cout << "SetCryptoKey : epoch:" << current_epoch << "\n";
}


sframe::bytes EncryptPipe::protect(const std::unique_ptr<Packet>& packet, uint16_t payloadSize)
{
	assert(current_epoch >= 0);

	gsl::span<const uint8_t> buffer_ref = packet->buffer;
	// encrypt from header onwards
	auto plaintext = buffer_ref.subspan(packet->headerSize, payloadSize);
	auto ct_out = sframe::bytes(plaintext.size() + sframe::max_overhead);
	auto ct = mls_context.protect(current_epoch, packet->name.senderID, ct_out, plaintext);
	// TODO see if we can reuse ct_out
	return to_bytes(ct);
}

sframe::bytes  EncryptPipe::unprotect(const std::unique_ptr<Packet>& packet, uint16_t payloadSize)
{

	gsl::span<uint8_t> buffer_ref = packet->buffer;
	// start decryption from data (excluding header)
	auto ciphertext = buffer_ref.subspan(packet->headerSize, payloadSize);
	auto pt_out = sframe::bytes(ciphertext.size());
	auto pt = mls_context.unprotect(pt_out, ciphertext);
	// TODO see if we can reuse pt_out
	return to_bytes(pt);
}