
#include <iostream>
#include <thread>

#include "encode.hh"
#include "qbroadcast.hh"

using namespace MediaNet;
bool dont_send_synack = false;

Connection::Connection(uint32_t relaySeq, uint64_t cookie_in)
: relaySeqNum(relaySeq)
 ,cookie(cookie_in) {}

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

  if (packet->fullSize() < 1) {
    // log bad packet
    return;
  }

	if (packet->fullData() == packetTagTrunc(PacketTag::headerMagicSyn)) {
    processSyn(packet);
    return;
  }

	if (packet->fullData() == packetTagTrunc(PacketTag::headerMagicRst)) {
		processRst(packet);
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

  std::clog << std::endl << "Got bad packet: Tag" << int(nextTag(packet)) << std::endl;
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

  // TODO - get rid of prev Ack tag and use ack vector
  if (prevAckSeqNum > 0) {
    NetAck prevAckTag{};
    prevAckTag.netAckSeqNum = prevAckSeqNum;
    prevAckTag.netRecvTimeUs = prevRecvTimeUs;
    ack << prevAckTag;
  }

  NetAck ackTag{};
  ackTag.netAckSeqNum = seqNumTag.clientSeqNum;
  ackTag.netRecvTimeUs = nowUs;
  ack << ackTag;

  transport.send(move(ack));

  prevAckSeqNum = ackTag.netAckSeqNum;
  prevRecvTimeUs = ackTag.netRecvTimeUs;

  // TODO - loop over connections and remove ones with old last Syn time

  // for each connection, make copy and forward
  for (auto const &[addr, con] : connectionMap) {
    // auto subData = std::make_unique<Packet>();
    // subData->copy(*packet);

    auto subData = packet->clone(); // TODO - just clone header stuff
    // subData->resize(0);

    subData->setDst(addr);

    NetRelaySeqNum netRelaySeqNum;
    netRelaySeqNum.relaySeqNum = con->relaySeqNum++;
    netRelaySeqNum.remoteSendTimeUs = nowUs;

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

	NetSyncReq sync = {};
	packet >> sync;

	auto conIndex = connectionMap.find(packet->getSrc());
	if(conIndex != connectionMap.end()) {
		// existing connection
		std::unique_ptr<Connection> &con = connectionMap[packet->getSrc()];
		con->lastSyn = std::chrono::steady_clock::now();
		if (dont_send_synack) {
			return;
		}
		std::clog << "existing connection\n";
		// send synAck
		// TODO: use proper values
		auto syncAckPkt = std::make_unique<Packet>();
		auto syncAck = NetSyncAck{};
		syncAck.serverTimeMs = 0; // TODO fix this
		syncAck.authSecret = sync.authSecret; // TODO: this needs to set OOB, not copied
		syncAckPkt << PacketTag::headerMagicSynAck;
		syncAckPkt << syncAck;
		syncAckPkt->setDst(packet->getSrc());
		transport.send(std::move(syncAckPkt));
		return;
	}

	// new connection or retry with cookie
  auto it = cookies.find(packet->getSrc());
  if (it == cookies.end()) {
  	// send a reset with retry cookie
  	auto cookie = random();
  	cookies.emplace(packet->getSrc(),
									 std::make_tuple(std::chrono::steady_clock::now(), cookie));

  	auto rstPkt = std::make_unique<Packet>();
		NetRstRetry rstRetry{};
		rstRetry.cookie = cookie;
		rstPkt << PacketTag::headerMagicRst;
		rstPkt << rstRetry;
		rstPkt->setDst(packet->getSrc());
		transport.send(std::move(rstPkt));
		std::clog << "new connection attempt, generate cookie:" << cookie << std::endl;
		return;
  }
  // cookie exists, verify it matches
  auto& [when, cookie] = it->second;
  // TODO: verify now() - when is within in acceptable limits
 	std::clog << "Cookie:" << sync.cookie << ", Version:" << sync.versionVec << std::endl;
	if (sync.cookie != cookie) {
		// bad cookie, reset the connection
		auto rstPkt = std::make_unique<Packet>();
		rstPkt << PacketTag::headerMagicRst;
		rstPkt->setDst(packet->getSrc());
		transport.send(std::move(rstPkt));
		std::clog << "incorrect cookie: found:" << sync.cookie << ", expected:" << cookie<< std::endl;
		return;
	}

	// good sync new connection
	connectionMap[packet->getSrc()] = std::make_unique<Connection>(getRandom(), cookie);
	cookies.erase(it);
	std::clog << "Added connection\n";
	auto syncAckPkt = std::make_unique<Packet>();
	auto syncAck = NetSyncAck{};
	syncAck.serverTimeMs = 0; // TODO fix this
	syncAck.authSecret = sync.authSecret;
	syncAckPkt << PacketTag::headerMagicSynAck;
	syncAckPkt << syncAck;
	syncAckPkt->setDst(packet->getSrc());
	transport.send(std::move(syncAckPkt));
}

void BroadcastRelay::processRst(std::unique_ptr<MediaNet::Packet> &packet) {
	auto conIndex = connectionMap.find(packet->getSrc());
	if (conIndex == connectionMap.end()) {
		std::clog << "Reset recieved for unknown connection\n";
		return;
	}
	connectionMap.erase(conIndex);
	std::clog << "Reset received for connection: " << IpAddr::toString(packet->getSrc()) << "\n";
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
