#pragma once

#include <chrono>
#include <functional>
#include <map>
#include <memory>
#include <random>

#include "fib.hh"
#include "packet.hh"
#include "quicRServer.hh"

class Relay {

public:
  explicit Relay(uint16_t port);
  void process();
  void stop();

private:
  void processAppMessage(std::unique_ptr<MediaNet::Packet> &packet);
  void processRateRequest(std::unique_ptr<MediaNet::Packet> &packet);
  void processSub(std::unique_ptr<MediaNet::Packet> &packet,
                  MediaNet::ClientData &clientSeqNum);
  void processPub(std::unique_ptr<MediaNet::Packet> &packet,
                  MediaNet::ClientData &clientSeqNum);

  uint32_t prevAckSeqNum = 0;
  uint32_t prevRecvTimeUs = 0;

  MediaNet::QuicRServer qServer;
  std::unique_ptr<Fib> fib;

  std::mt19937 randomGen;
  std::uniform_int_distribution<uint32_t> randomDist;
  std::function<uint32_t()> getRandom;
};
