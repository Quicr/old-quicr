
#include <chrono>
#include <thread>
#include <random>
#include <functional>
#include <cassert>


#include "../include/relay.hh"
#include "../include/multimap_fib.hh"

using namespace MediaNet;

///
/// Connection
///

Connection::Connection(uint32_t relaySeqNo) : relaySeqNum(relaySeqNo) {}


Relay::Relay(uint16_t port)
		: transport(* new UdpPipe)
		  , fib(std::make_unique<MultimapFib>()) {
	transport.start(port, "", nullptr);
	std::random_device randDev;
	randomGen.seed(randDev()); // TODO - should use crypto random
	getRandom = std::bind(randomDist, randomGen);
}

void Relay::process() {
	auto packet = transport.recv();

	if(!packet) {
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
		return;
	}

	auto tag = nextTag(packet);

	switch (tag) {
		case PacketTag::sync:
			return processSyn(packet);
		case PacketTag::clientSeqNum:
			return processAppMessage(packet);
		case PacketTag::relayRateReq:
			return processRateRequest(packet);
		default:
			std::clog << "unknown tag :" << (int) tag << "\n";
	}

}


///
/// Private Implementation
///

void Relay::processSyn(std::unique_ptr<MediaNet::Packet> &packet) {
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

void Relay::processAppMessage(std::unique_ptr<MediaNet::Packet>& packet) {
	NetClientSeqNum seqNumTag{};
	packet >> seqNumTag;

	//auto tag = PacketTag::none;
	//packet >> tag;
	auto tag = nextTag(packet);
	if(tag == PacketTag::appData) {
		return processPub(packet, seqNumTag);
	} else if(tag  == PacketTag::subscribeReq) {
		return processSub(packet, seqNumTag);
	}

	std::clog << "Bad App message:" << (int) tag << "\n";
}

/// Subscribe Request
void Relay::processSub(std::unique_ptr<MediaNet::Packet> &packet, NetClientSeqNum& clientSeqNumTag) {
	std::chrono::steady_clock::time_point tp = std::chrono::steady_clock::now();
	std::chrono::steady_clock::duration dn = tp.time_since_epoch();
	uint32_t nowUs =
					(uint32_t)std::chrono::duration_cast<std::chrono::microseconds>(dn)
									.count();

	// ack the packet
	auto ack = std::make_unique<Packet>();
	ack->setDst(packet->getSrc());
	ack << PacketTag::headerMagicData;
	NetAck ackTag{};
	ackTag.netAckSeqNum = clientSeqNumTag.clientSeqNum;
	ackTag.netRecvTimeUs = nowUs;
	ack << ackTag;

	// save the subscription
	PacketTag tag;
	packet >> tag;
  ShortName name;
  packet >> name;
  std::clog << "Adding Subscription for: " << name << std::endl;
  fib->addSubscription(name, SubscriberInfo{name, packet->getSrc()});
}

void Relay::processPub(std::unique_ptr<MediaNet::Packet> &packet, NetClientSeqNum& clientSeqNumTag) {
	std::chrono::steady_clock::time_point tp = std::chrono::steady_clock::now();
	std::chrono::steady_clock::duration dn = tp.time_since_epoch();
	uint32_t nowUs =
					(uint32_t)std::chrono::duration_cast<std::chrono::microseconds>(dn)
									.count();

	std::clog << ".";

	// save the name for publish
	PacketTag tag;
	packet >> tag;
	ShortName name;
	packet >> name;

  uint16_t payloadSize;
	packet >> payloadSize;
	if (payloadSize > packet->size()) {
		std::clog << "relay recv bad data size " << payloadSize << " "
							<< packet->size() << std::endl;
		return;
	}

	// TODO: refactor ack logic
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
	ackTag.netAckSeqNum = clientSeqNumTag.clientSeqNum;
	ackTag.netRecvTimeUs = nowUs;
	ack << ackTag;

	transport.send(move(ack));

	prevAckSeqNum = ackTag.netAckSeqNum;
	prevRecvTimeUs = ackTag.netRecvTimeUs;

  // find the matching subscribers
  auto subscribers = fib->lookupSubscription(name);

  // std::clog << "Name:" << name << " has:" << subscribers.size() << " subscribers\n";

  packet << payloadSize;
  packet << name;
  packet << tag;

  for(auto const& subscriber : subscribers) {
		auto subData = packet->clone(); // TODO - just clone header stuff
		auto con = connectionMap.find(subscriber.face);
		assert(con != connectionMap.end());
		subData->setDst(subscriber.face);

		NetRelaySeqNum netRelaySeqNum{};
		netRelaySeqNum.relaySeqNum = con->second->relaySeqNum++;
		netRelaySeqNum.remoteSendTimeUs = nowUs;

		subData << netRelaySeqNum;

		// std::clog << "Relay send: " << subData->size() << std::endl;

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


void Relay::processRateRequest(std::unique_ptr<MediaNet::Packet> &packet) {
	NetRateReq rateReq{};
	packet >> rateReq;
	std::clog << std::endl
						<< "ReqRate: " << float(rateReq.bitrateKbps) / 1000.0 << " mbps"
						<< std::endl;
}

void Relay::stop() {
  assert(0);
  // TODO
}
