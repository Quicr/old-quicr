#pragma once

#include <chrono>
#include <cstdint>

#include "packet.hh"

using namespace MediaNet;

namespace MediaNet {

struct PacketUpstreamStatus {
  uint32_t seqNum;
  uint32_t sendTimeUs;
  uint16_t sizeBits;
  uint32_t sendPhaseCount; // cycleCount * numPhasePerCycle + phase
  bool notLost;
  // bool congested;
  uint32_t remoteAckTimeUs;
  uint32_t localRecvAckTimeUs;
};

struct PacketDownstreamStatus {
  uint32_t relaySeqNum;
  uint32_t receiveTimeUs;
  uint32_t remoteSendTimeUs;
  uint16_t sizeBits;
  uint32_t sendPhaseCount; // cycleCount * numPhasePerCycle + phase
  bool notLost;
  // bool congested;
};

class RateCtrl {
public:
  RateCtrl();

  void sendPacket(uint32_t seqNum, uint32_t sendTimeUs, uint16_t sizeBits);
  void recvPacket(uint32_t relaySeqNum, uint32_t remoteSendTimeUs,
                  uint32_t localRecvTimeUs, uint16_t sizeBits);
  void recvAck(uint32_t seqNum, uint32_t remoteAckTimeUs,
               uint32_t localRecvAckTimeUs);

  uint32_t rttEstUs() const; // in microseconds

  uint64_t bwUpEst() const;   // in bits per second
  uint64_t bwDownEst() const; // in bits per second

  uint32_t getPhase() const { return phase; }
  uint64_t bwUpTarget() const;   // in bits per second
  uint64_t bwDownTarget() const; // in bits per second

private:
  uint32_t upHistorytSeqOffset;
  std::vector<PacketUpstreamStatus> upstreamHistory;

  uint32_t downHistorytSeqOffset;
  std::vector<PacketDownstreamStatus> downstreamHistory;

  void updatePhase();
  const uint32_t phaseTimeUs = 33333 * 2; // 2 frames at  30 fps
  const uint32_t numPhasePerCycle = 10;

  void startNewPhase();
  uint32_t phase; // resets to zero with each new cycle
  uint32_t bitsSentThisPhase;

  void startNewCycle();
  std::chrono::steady_clock::time_point cycleStartTime;
  uint32_t cycleCount;

  void updateRTT(uint32_t valUs);
  uint32_t minCycleRTTUs;
  uint32_t maxCycleAckTimeUs;
  uint32_t estRTTUs;
  uint32_t estAckTimeUs;

  void estUpstreamBw();
  float upstreamPacketLossRate; // probability between 0 and 1

  void updateUpstreamBwFilter(float bps, float lossRate, float delayMs);
  float upstreamCycleMaxBw; // in bps
  float upstreamBwEst;      // in bps

  void estDownstreamBw();
  float downstreamPacketLossRate; // probability between 0 and 1

  void updateDownstreamBwFilter(float bps, float lossRate, float delayMs);
  float downstreamCycleMaxBw; // in bps
  float downstreamBwEst;      // in bps

  // void sendDownstreamRateMsg();
};

} // namespace MediaNet
