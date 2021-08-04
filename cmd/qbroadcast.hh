#pragma once

#include <chrono>
#include <functional> // for bind
#include <map>
#include <random>

#include "../src/encode.hh" // TODO

#include "quicr/packet.hh"
#include "quicr/quicRServer.hh"

class Connection
{
public:
  Connection(uint32_t relaySeq, const MediaNet::IpAddr& addr);
  uint32_t relaySeqNum;
  MediaNet::IpAddr address;
  std::chrono::time_point<std::chrono::steady_clock> lastSyn;
};

class BroadcastRelay
{
public:
  explicit BroadcastRelay(uint16_t port);
  void process();
  void processAppMessage(std::unique_ptr<MediaNet::Packet>& packet);
  void processPub(std::unique_ptr<MediaNet::Packet>& packet,
                  MediaNet::ClientData& clientSeqNumTa);
  void processSub(std::unique_ptr<MediaNet::Packet>& packet,
                  MediaNet::ClientData& clientSeqNum);
  void processRate(std::unique_ptr<MediaNet::Packet>& packet);

private:
  uint32_t prevAckSeqNum = 0;
  uint32_t prevRecvTimeUs = 0;
  MediaNet::QuicRServer qServer;
  std::map<MediaNet::ShortName, std::unique_ptr<Connection>> connectionMap;

  std::mt19937 randomGen;
  std::uniform_int_distribution<uint32_t> randomDist;
  std::function<uint32_t()> getRandom;
};