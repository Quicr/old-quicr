#pragma once

#include <map>
#include <random>
#include <functional>
#include <chrono>
#include <memory>

#include "packet.hh"
#include "fib.hh"
#include "quicRServer.hh"


class Connection {
public:
	Connection(uint32_t relaySeqNo);
	uint32_t relaySeqNum;
	std::chrono::time_point<std::chrono::steady_clock> lastSyn;

};
>>>>>>> 85487b98cea0dd2044c9acda28a7316a912928c7

class Relay {

public:
   explicit Relay(uint16_t port);
	 void process();
	 void stop();

private:
	void processAppMessage(std::unique_ptr<MediaNet::Packet>& packet);
	void processRateRequest(std::unique_ptr<MediaNet::Packet>& packet);
	void processSub(std::unique_ptr<MediaNet::Packet>& packet, MediaNet::NetClientSeqNum& clientSeqNum);
	void processPub(std::unique_ptr<MediaNet::Packet>& packet, MediaNet::NetClientSeqNum& clientSeqNum);

	void sendSyncRequest(const MediaNet::IpAddr& to, uint64_t authSecret);

	uint32_t prevAckSeqNum = 0;
	uint32_t prevRecvTimeUs = 0;

	QuicRServer qServer;
	std::unique_ptr<Fib> fib;

	std::mt19937 randomGen;
	std::uniform_int_distribution<uint32_t> randomDist;
	std::function<uint32_t()> getRandom;
};

