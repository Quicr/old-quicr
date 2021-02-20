
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
      statsPipe(&encryptPipe), firstPipe(&statsPipe) {
    // TODO - get rid of all other places were defaults get set for mtu, rtt, pps
    firstPipe->updateMTU(1280,480 );
    firstPipe->updateRTT(20,50);
}

QuicRClient::~QuicRClient() { firstPipe->stop(); }

bool QuicRClient::ready() const { return firstPipe->ready(); }

void QuicRClient::close() { firstPipe->stop(); }

void QuicRClient::setCryptoKey(sframe::MLSContext::EpochID epoch, const sframe::bytes &mls_epoch_secret) {
	encryptPipe.setCryptoKey(epoch, mls_epoch_secret);
}


bool QuicRClient::publish(std::unique_ptr<Packet> packet) {
  size_t payloadSize = packet->size();
  assert(payloadSize < 63 * 1200);
  packet << (uint16_t)payloadSize;
  packet << packet->name;
  packet << PacketTag::appData;
  return firstPipe->send(move(packet));
}

std::unique_ptr<Packet> QuicRClient::recv() {

  auto packet = std::unique_ptr<Packet>(nullptr);

  bool bad = true;
  while (bad) {

    packet = firstPipe->recv();

    if (!packet) {
      return packet;
    }

    if (packet->size() < 1) {
      // TODO log bad data
      std::clog << "quicr recv very bad size = " << packet->size() << std::endl;
      continue;
    }

    PacketTag tag;
    packet >> tag;

    if (tag == PacketTag::headerMagicData) {
      // std::clog << "quicr empty message " << std::endl;
      continue;
    }

    if (tag != PacketTag::appData) {
      // TODO log bad data
      std::clog << "quicr recv bad tag: " << (((uint16_t)(tag)) >> 8)
                << std::endl;
      continue;
    }

    if (packet->size() < 2) {
      // TODO log bad data
      std::clog << "quicr recv bad size=" << packet->size() << std::endl;
      continue;
    }

    ShortName name;
    packet >> name;
		//std::clog << "quicr recv data from " << name << std::endl;

		uint16_t payloadSize;
    packet >> payloadSize;
    if (payloadSize > packet->size()) {
      std::clog << "quicr recv bad data size " << payloadSize << " "
                << packet->size() << std::endl;
      continue;
    }

    packet->headerSize = (int)(packet->fullSize()) - payloadSize;

    bad = false;
  }

  // std::clog << "QuicR received packet size=" << packet->size() << std::endl;
  return packet;
}

bool QuicRClient::open(uint32_t clientID, const std::string relayName,
                       const uint16_t port, uint64_t token) {
  (void)clientID; // TODO
  (void)token;    // TODO
  connectionPipe.setAuthInfo(clientID, token);

  return firstPipe->start(port, relayName, nullptr);
}

uint64_t QuicRClient::getTargetUpstreamBitrate() {
  return pacerPipe.getTargetUpstreamBitrate(); // TODO - move to stats
}

std::unique_ptr<Packet> QuicRClient::createPacket(const ShortName &shortName,
                                                  int reservedPayloadSize) {
  auto packet = std::make_unique<Packet>();
  assert(packet);
  packet->name = shortName;
  packet->reserve(reservedPayloadSize + 20); // TODO - tune the 20

  packet << PacketTag::headerMagicData;
  // packet << shortName;
  // packet << PacketTag::extraMagicVer1;

  packet->headerSize = (int)(packet->buffer.size());

  // std::clog << "Create empty packet " << *packet << std::endl;

  return packet;
}

bool QuicRClient::subscribe(ShortName name) {
  return subscribePipe.subscribe(name);
}
