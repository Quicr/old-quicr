
#include <iostream>
#include <thread>

#include "encode.hh"
#include "qbroadcast.hh"

using namespace MediaNet;

Connection::Connection(uint32_t randNum) : relaySeqNum(randNum) {}

BroadcastRelay::BroadcastRelay(uint16_t port) : transport(*new UdpPipe) {
  transport.start(port, "", nullptr);
  prevAckSeqNum = 0;
  prevRecvTimeUs = 0;

  std::random_device randDev;
  randomGen.seed(randDev()); // TODO - should use crypto random
  getRandom = std::bind(randomDist, randomGen);
}

void BroadcastRelay::process() {
  std::unique_ptr<Packet> packet = transport.recv();

  if (!packet) {
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    return;
  }

  if (packet->fullSize() <= 1) {
    // log bad packet
    return;
  }

  if (packet->fullData() == packetTagTrunc(PacketTag::headerMagicSyn)) {
    processSyn(packet);
    return;
  }

  if (nextTag(packet) == PacketTag::clientSeqNum) {
    processPub(packet);
    return;
  }

  if (nextTag(packet) == PacketTag::relayRateReq) {
    // std::clog << std::endl << "Got relayRateReq" << std::endl;
    processRate(packet);
    return;
  }

  std::clog << std::endl << "Got bad packet" << std::endl;
}

void BroadcastRelay::processRate(std::unique_ptr<MediaNet::Packet> &packet) {
  NetRateReq rateReq;
  packet >> rateReq;
  std::clog << std::endl
            << "ReqRate: " << float(rateReq.bitrateKbps) / 1000.0 << " mbps"
            << std::endl;
}

void BroadcastRelay::processPub(std::unique_ptr<MediaNet::Packet> &packet) {
  std::chrono::steady_clock::time_point tp = std::chrono::steady_clock::now();
  std::chrono::steady_clock::duration dn = tp.time_since_epoch();
  uint32_t nowUs =
      (uint32_t)std::chrono::duration_cast<std::chrono::microseconds>(dn)
          .count();

  NetClientSeqNum seqNumTag;
  packet >> seqNumTag;

  std::clog << ".";

  // std::clog << int(seqNumTag.clientSeqNum);

  auto ack = std::make_unique<Packet>();
  ack->setDst(packet->getSrc());

  ack << PacketTag::headerMagicData;

  if (prevAckSeqNum > 0) {
    NetAck prevAckTag{};
    prevAckTag.netAckSeqNum = prevAckSeqNum;
    prevAckTag.netRecvTimeUs = prevRecvTimeUs;
    ack << prevAckTag;
  }

  NetAck ackTag{};
  ackTag.netAckSeqNum = (seqNumTag.clientSeqNum);
  ackTag.netRecvTimeUs = nowUs;
  ack << ackTag;

  transport.send(move(ack));

  prevAckSeqNum = (seqNumTag.clientSeqNum);
  prevRecvTimeUs = nowUs;

  // TODO - loop over connections and remove ones with old last Syn time

  // for each connection, make copy and forward
  for (auto const &[addr, con] : connectionMap) {
    // auto subData = std::make_unique<Packet>();
    // subData->copy(*packet);

    auto subData = packet->clone(); // TODO - just clone header stuff
    // subData->resize(0);

    subData->setDst(addr);

    NetRelaySeqNum netRelaySeqNum;
    netRelaySeqNum.relaySeqNum = con->remoteSeqNum++;
    netRelaySeqNum.remoteSendTimeMs = nowUs / 1000;

    subData << netRelaySeqNum;

    // std::clog << "Relay send: " << *subData << std::endl;

    bool simLoss = false;

    if (false) {
      //  simulate 10% packet loss
      if ((netRelaySeqNum.relaySeqNum % 10) == 7) {
        simLoss = true;
      }
    }

    if (!simLoss) {
      transport.send(move(subData));
      std::clog << "*";
    } else {
      std::clog << "-";
    }
  }
}

void BroadcastRelay::processSyn(std::unique_ptr<MediaNet::Packet> &packet) {
  std::clog << "Got a Syn"
            << " from=" << IpAddr::toString(packet->getSrc())
            << " len=" << packet->fullSize() << std::endl;

  auto conIndex = connectionMap.find(packet->getSrc());
  if (conIndex == connectionMap.end()) {
    // new connection
    connectionMap[packet->getSrc()] = std::make_unique<Connection>(getRandom());
  }

  std::unique_ptr<Connection> &con = connectionMap[packet->getSrc()];
  con->lastSyn = std::chrono::steady_clock::now();
}

#ifdef __clang__
#pragma clang diagnostic ignored "-Wunknown-pragmas"
#endif
#ifdef _MSC_VER
#pragma warning(disable : 4068)
#endif

#pragma ide diagnostic ignored "EndlessLoop"
#pragma ide diagnostic ignored "UnreachableCode"

int main() {
  BroadcastRelay qRelay(5004);

  while (true) {
    qRelay.process();
  }
  return 0;
}
