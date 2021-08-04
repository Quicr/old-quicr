
#include <cassert>
#include <chrono>
#include <functional>
#include <random>
#include <thread>

#include "../include/multimap_fib.hh"
#include "../include/relay.hh"

using namespace MediaNet;

Relay::Relay(uint16_t port)
  : qServer()
  , fib(std::make_unique<MultimapFib>())
{
  qServer.open(port);
  std::random_device randDev;
  randomGen.seed(randDev()); // TODO - should use crypto random
  getRandom = std::bind(randomDist, randomGen);
}

void
Relay::process()
{
  auto packet = qServer.recv();

  if (!packet) {
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    return;
  }

  auto tag = nextTag(packet);

  switch (tag) {
    case PacketTag::clientData:
      return processAppMessage(packet);
    case PacketTag::rate:
      return processRateRequest(packet);
    default:
      std::clog << "unknown tag :" << (int)tag << "\n";
  }
}

///
/// Private Implementation
///

void
Relay::processAppMessage(std::unique_ptr<MediaNet::Packet>& packet)
{
  ClientData seqNumTag{};
  packet >> seqNumTag;

  // auto tag = PacketTag::none;
  // packet >> tag;
  auto tag = nextTag(packet);
  if (tag == PacketTag::clientData) {
    return processPub(packet, seqNumTag);
  } else if (tag == PacketTag::subscribe) {
    return processSub(packet, seqNumTag);
  }

  std::clog << "Bad App message:" << (int)tag << "\n";
}

/// Subscribe Request
void
Relay::processSub(std::unique_ptr<MediaNet::Packet>& packet,
                  ClientData& clientSeqNumTag)
{
  std::chrono::steady_clock::time_point tp = std::chrono::steady_clock::now();
  std::chrono::steady_clock::duration dn = tp.time_since_epoch();
  uint32_t nowUs =
    (uint32_t)std::chrono::duration_cast<std::chrono::microseconds>(dn).count();

  // ack the packet
  auto ackPacket = std::make_unique<Packet>();
  ackPacket->setDst(packet->getSrc());
  auto hdr = Packet::Header(PacketTag::headerData);
  ackPacket << hdr;
  NetAck ack{};
  ack.clientSeqNum = clientSeqNumTag.clientSeqNum;
  ack.recvTimeUs = nowUs;
  ackPacket << ack;

  // save the subscription
  PacketTag tag;
  packet >> tag;
  ShortName name;
  packet >> name;
  std::clog << "Adding Subscription for: " << name << std::endl;
  fib->addSubscription(name,
                       SubscriberInfo{ name, packet->getSrc(), getRandom() });
}

void
Relay::processPub(std::unique_ptr<MediaNet::Packet>& packet,
                  ClientData& clientSeqNumTag)
{
  std::chrono::steady_clock::time_point tp = std::chrono::steady_clock::now();
  std::chrono::steady_clock::duration dn = tp.time_since_epoch();
  uint32_t nowUs =
    (uint32_t)std::chrono::duration_cast<std::chrono::microseconds>(dn).count();

  std::clog << ".";

  // save the name for publish
  ClientData clientData;
  NamedDataChunk namedDataChunk;
  EncryptedDataBlock encryptedDataBlock;

  bool ok = true;

  ok &= packet >> clientData;
  ok &= packet >> namedDataChunk;
  ok &= packet >> encryptedDataBlock;

  assert(fromVarInt(encryptedDataBlock.metaDataLen) == 0); // TODO

  uint16_t payloadSize = fromVarInt(encryptedDataBlock.cipherDataLen);
  if (payloadSize > packet->size()) {
    std::clog << "relay recv bad data size " << payloadSize << " "
              << packet->size() << std::endl;
    return;
  }

  // TODO: refactor ack logic
  auto ack = std::make_unique<Packet>();
  ack->setDst(packet->getSrc());
  // TODO: set the token in connPipe once qServer uses
  // ConnectionPipe
  auto token = packet->getPathToken();
  auto hdr = Packet::Header(PacketTag::headerData, token);
  ack << hdr;

  // TODO - get rid of prev Ack tag and use ack vector
  if (prevAckSeqNum > 0) {
    NetAck prevAckTag{};
    prevAckTag.clientSeqNum = prevAckSeqNum;
    prevAckTag.recvTimeUs = prevRecvTimeUs;
    ack << prevAckTag;
  }

  NetAck ackTag{};
  ackTag.clientSeqNum = clientSeqNumTag.clientSeqNum;
  ackTag.recvTimeUs = nowUs;
  ack << ackTag;

  qServer.send(move(ack));

  prevAckSeqNum = ackTag.clientSeqNum;
  prevRecvTimeUs = ackTag.recvTimeUs;

  // find the matching subscribers
  auto subscribers = fib->lookupSubscription(namedDataChunk.shortName);

  std::clog << "Name:" << namedDataChunk.shortName << " has:" << subscribers.size() << "subscribers\n";

  packet << encryptedDataBlock;
  packet << namedDataChunk;

  for (auto& subscriber : subscribers) {
    auto relayDataPacket = packet->clone(); // TODO - just clone header stuff
    relayDataPacket->setDst(subscriber.face);
    RelayData relayData{};
    relayData.relaySeqNum = subscriber.relaySeqNum++;
    relayData.relaySendTimeUs = nowUs;

    relayDataPacket << relayData;

    std::clog << "Relay send: " << relayDataPacket->size() << std::endl;

    bool simLoss = false;

    if (false) {
      //  simulate 10% packet loss
      if ((relayData.relaySeqNum % 10) == 7) {
        simLoss = true;
      }
    }

    if (!simLoss) {
      qServer.send(move(relayDataPacket));
      std::clog << "*";
    } else {
      std::clog << "-";
    }
  }
}

void
Relay::processRateRequest(std::unique_ptr<MediaNet::Packet>& packet)
{
  NetRateReq rateReq{};
  packet >> rateReq;
  std::clog << std::endl
            << "ReqRate: " << float(rateReq.bitrateKbps) / 1000.0 << " mbps"
            << std::endl;
}

void
Relay::stop()
{
  assert(0);
  // TODO
}
