#pragma once

#include <map>
#include <random>
#include <functional>
#include <chrono>
#include <memory>

#include "packet.hh"
#include "udpPipe.hh"
#include "fib.hh"


class Connection {
public:
	Connection(uint32_t relaySeqNo);
	uint32_t relaySeqNum;
	std::chrono::time_point<std::chrono::steady_clock> lastSyn;

};

class Relay {

public:
   explicit Relay(uint16_t port);
	 void process();
	 void stop();

private:
	void processSyn(std::unique_ptr<MediaNet::Packet>& packet);
	void processAppMessage(std::unique_ptr<MediaNet::Packet>& packet);
	void processRateRequest(std::unique_ptr<MediaNet::Packet>& packet);
	void processSub(std::unique_ptr<MediaNet::Packet>& packet, MediaNet::ClientData& clientSeqNum);
	void processPub(std::unique_ptr<MediaNet::Packet>& packet, MediaNet::ClientData& clientSeqNum);


	uint32_t prevAckSeqNum = 0;
	uint32_t prevRecvTimeUs = 0;

	MediaNet::UdpPipe& transport;
	std::map<MediaNet::IpAddr, std::unique_ptr<Connection>> connectionMap;
	std::unique_ptr<Fib> fib;
	// TODO revisit this
	std::mt19937 randomGen;
	std::uniform_int_distribution<uint32_t> randomDist;
	std::function<uint32_t()> getRandom;
};

