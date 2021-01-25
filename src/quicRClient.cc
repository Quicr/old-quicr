
#include <cassert>

#include "encode.hh"
#include "quicRClient.hh"

#include "connectionPipe.hh"
#include "crazyBitPipe.hh"
#include "encryptPipe.hh"
#include "fecPipe.hh"
#include "fragmentPipe.hh"
#include "pacerPipe.hh"
#include "priorityPipe.hh"
#include "retransmitPipe.hh"
#include "subscribePipe.hh"
#include "udpPipe.hh"

using namespace MediaNet;

QuicRClient::QuicRClient()
    : udpPipe(), fakeLossPipe(&udpPipe), crazyBitPipe(&fakeLossPipe),
      connectionPipe(&crazyBitPipe), pacerPipe(&connectionPipe),
      priorityPipe(&pacerPipe), retransmitPipe(&priorityPipe),
      fecPipe(&retransmitPipe), subscribePipe(&fecPipe),
      fragmentPipe(&subscribePipe), encryptPipe(&fragmentPipe),
      statsPipe( &encryptPipe ), 
      firstPipe(&statsPipe), pubClientID(0), secToken(0) {}

QuicRClient::~QuicRClient() { close(); }

bool QuicRClient::ready() const { return firstPipe->ready(); }

void QuicRClient::close() { firstPipe->stop(); }

bool QuicRClient::publish(std::unique_ptr<Packet> packet) {
  size_t payloadSize = packet->buffer.size() - packet->headerSize;
  assert(payloadSize < 1200); // TODO
  packet << (uint16_t)payloadSize;
  packet << PacketTag::appData;
  return firstPipe->send(move(packet));
}

std::unique_ptr<Packet> QuicRClient::recv() {
  auto packet = firstPipe->recv();
  return packet;
}

bool QuicRClient::open(uint32_t clientID, const std::string relayName,
                       const uint16_t port, uint64_t token) {
  pubClientID = clientID;
  secToken = token;

  connectionPipe.setAuthInfo(clientID, token);

  return firstPipe->start(port, relayName, nullptr);
}

uint64_t QuicRClient::getTargetUpstreamBitrate() {
  return pacerPipe.getTargetUpstreamBirate();
}

std::unique_ptr<Packet>
QuicRClient::createPacket(const Packet::ShortName &shortName,
                          int reservedPayloadSize) {
  auto packet = std::make_unique<Packet>();
  assert(packet);
  packet->name = shortName;
  packet->buffer.reserve(reservedPayloadSize + 20); // TODO - tune the 20

  packet << PacketTag::headerMagicData;
  packet << shortName;
  // packet << PacketTag::extraMagicVer1;

  packet->headerSize = (int)( packet->buffer.size() );

  return packet;
}

bool QuicRClient::subscribe(Packet::ShortName name) {
    return subscribePipe.subscribe( name );
}
