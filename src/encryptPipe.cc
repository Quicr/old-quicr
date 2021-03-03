

#include "encryptPipe.hh"
#include "packet.hh"
#include <cassert>
#include <iostream>
#include <sstream>

using namespace MediaNet;

// TODO Sframe parameters needs to be retrieved from MLS
static const auto FIXED_CIPHER_SUITE = sframe::CipherSuite::AES_GCM_128_SHA256;
static const size_t SFRAME_EPOCH_BITS = 8;

template <typename T> sframe::bytes static to_bytes(const T &range) {
  return sframe::bytes(range.begin(), range.end());
}

EncryptPipe::EncryptPipe(PipeInterface *t)
    : PipeInterface(t), mls_context(FIXED_CIPHER_SUITE, SFRAME_EPOCH_BITS) {}

bool EncryptPipe::send(std::unique_ptr<Packet> packet) {

  // TODO: figure out right AAD bits
  assert(downStream);
  // return downStream->send(std::move(packet));

  ClientData clientData;
  NamedDataChunk namedDataChunk;
  DataBlock dataBlock;

  bool ok = true;
  assert(nextTag(packet) == PacketTag::clientData);
  ok &= packet >> clientData;
  ok &= packet >> namedDataChunk;
  assert( ok );
  assert(nextTag(packet) == PacketTag::dataBlock);
  ok &= packet >> dataBlock;
  assert( ok );

  assert( fromVarInt( dataBlock.metaDataLen ) == 0 );
  uint16_t payloadSize = fromVarInt( dataBlock.dataLen );

  auto encrypted = protect(packet, payloadSize);
  // std::cout << "Payload Original Size/Encrypted Size:" << payloadSize << "/"
  // << encrypted.size() << "\n";

  // re-insert encrypted portion
  uint8_t *dst = &(packet->data());
  packet->resize( encrypted.size() );
  std::copy(encrypted.begin(), encrypted.end(), dst);

  EncryptedDataBlock encryptedDataBlock;
  encryptedDataBlock.metaDataLen = dataBlock.metaDataLen;
  encryptedDataBlock.cipherDataLen = toVarInt( encrypted.size() );
  encryptedDataBlock.authTagLen = 0 ;

  packet << encryptedDataBlock;
  packet << namedDataChunk;
  packet << clientData;

  // std::cout << "Full Encrypted Packet with header: "<< packet->size() << "
  // bytes\n";

  return downStream->send(move(packet));
}

std::unique_ptr<Packet> EncryptPipe::recv() {

  // TODO check packet integrity and decrypt

  assert(downStream);
  // return downStream->recv();

  auto packet = downStream->recv();
  if (packet) {
    auto tag = nextTag(packet);
    if (tag == PacketTag::shortName) {

      NamedDataChunk namedDataChunk;
      EncryptedDataBlock encryptedDataBlock;

      bool ok = true;

      ok &= packet >> namedDataChunk;
      ok &= packet >> encryptedDataBlock;

      if ( !ok ) {
        // todo should log bad packet
        return std::unique_ptr<Packet>(nullptr);
      }

      assert( fromVarInt( encryptedDataBlock.metaDataLen ) == 0 ); // TODO
      uint16_t payloadSize = fromVarInt( encryptedDataBlock.cipherDataLen );
      assert(payloadSize>0);

      auto decrypted = unprotect(packet, payloadSize);

      // TODO - how does decrypt auth error get hangled

      packet->resize( decrypted.size() );
      std::copy(decrypted.begin(), decrypted.end(), &packet->data());

      DataBlock dataBlock;
      dataBlock.metaDataLen = encryptedDataBlock.metaDataLen;
      dataBlock.dataLen = toVarInt( decrypted.size() );

      packet << dataBlock;
      packet << namedDataChunk;
      //packet << relayData;

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

sframe::bytes EncryptPipe::protect(const std::unique_ptr<Packet> &packet,
                                   uint16_t payloadSize) {
  assert(current_epoch >= 0);

  gsl::span<const uint8_t> buffer_ref = packet->buffer;
  // encrypt from header onwards
  auto plaintext = buffer_ref.subspan(packet->headerSize, payloadSize);
  auto ct_out = sframe::bytes(plaintext.size() + sframe::max_overhead);
  auto ct = mls_context.protect(current_epoch, packet->name.senderID, ct_out,
                                plaintext);
  // TODO see if we can reuse ct_out
  return to_bytes(ct);
}

sframe::bytes EncryptPipe::unprotect(const std::unique_ptr<Packet> &packet,
                                     uint16_t payloadSize) {

  gsl::span<uint8_t> buffer_ref = packet->buffer;
  // start decryption from data (excluding header)
  auto ciphertext = buffer_ref.subspan(packet->headerSize, payloadSize);
  auto pt_out = sframe::bytes(ciphertext.size());
  auto pt = mls_context.unprotect(pt_out, ciphertext);
  // TODO see if we can reuse pt_out
  return to_bytes(pt);
}