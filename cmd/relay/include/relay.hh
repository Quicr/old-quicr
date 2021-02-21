#pragma once

#include <map>
#include <random>
#include "packet.hh"
#include "udpPipe.hh"
#include "fib.hh"

class Connection {
public:
	Connection(uint32_t relaySeq, uint64_t cookie_in);
	// const MediaNet::IpAddr remoteAddr; // get from map
	uint32_t relaySeqNum;
	uint64_t  cookie;
	std::chrono::time_point<std::chrono::steady_clock> lastSyn;
};

class Relay {
	using timepoint = std::chrono::time_point<std::chrono::steady_clock>;

public:
   explicit Relay(uint16_t port);
	 void process();
	 void stop();

private:
	void processSyn(std::unique_ptr<MediaNet::Packet>& packet);
	void processRst(std::unique_ptr<MediaNet::Packet>& packet);
	void processAppMessage(std::unique_ptr<MediaNet::Packet>& packet);
	void processRateRequest(std::unique_ptr<MediaNet::Packet>& packet);
	void processSub(std::unique_ptr<MediaNet::Packet>& packet, MediaNet::NetClientSeqNum& clientSeqNum);
	void processPub(std::unique_ptr<MediaNet::Packet>& packet, MediaNet::NetClientSeqNum& clientSeqNum);

	void sendSyncRequest(const MediaNet::IpAddr& to, uint64_t serverTimeMs);

	uint32_t prevAckSeqNum = 0;
	uint32_t prevRecvTimeUs = 0;

	MediaNet::UdpPipe& transport;
	// TODO: need to timeout on the entries in this map to
	// avoid DOS attacks
	std::map<MediaNet::IpAddr, std::tuple<timepoint, uint32_t>> cookies;
	std::map<MediaNet::IpAddr, std::unique_ptr<Connection>> connectionMap;
	std::unique_ptr<Fib> fib;

	// TODO revisit this
	std::mt19937 randomGen;
	std::uniform_int_distribution<uint32_t> randomDist;
	std::function<uint32_t()> getRandom;
};

