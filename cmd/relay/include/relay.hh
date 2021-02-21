#pragma once

#include <map>
#include <random>
#include "packet.hh"
#include "udpPipe.hh"
#include "fib.hh"
#include "quicRServer.hh"

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

