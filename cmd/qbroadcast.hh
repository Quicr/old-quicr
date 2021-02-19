#pragma once

#include <chrono>
#include <functional> // for bind
#include <map>
#include <random>

#include "packet.hh"
#include "udpPipe.hh"

class Connection {
public:
  Connection(uint32_t randNum);
  // const MediaNet::IpAddr remoteAddr; // get from map
  uint32_t relaySeqNum;
  std::chrono::time_point<std::chrono::steady_clock> lastSyn;
};

class BroadcastRelay {
public:
  explicit BroadcastRelay(uint16_t port);
  void process();
  void processSyn(std::unique_ptr<MediaNet::Packet> &packet);
  void processPub(std::unique_ptr<MediaNet::Packet> &packet);
  void processRate(std::unique_ptr<MediaNet::Packet> &packet);
	void processRst(std::unique_ptr<MediaNet::Packet> &packet);

private:
  uint32_t prevAckSeqNum = 0;
  uint32_t prevRecvTimeUs = 0;
  MediaNet::UdpPipe &transport;

  std::map<MediaNet::IpAddr, std::unique_ptr<Connection>> connectionMap;

  std::mt19937 randomGen;
  std::uniform_int_distribution<uint32_t> randomDist;
  std::function<uint32_t()> getRandom;
};