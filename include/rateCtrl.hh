#pragma once

#include <chrono>
#include <cstdint>

#include "packet.hh"
#include "pipeInterface.hh"

using namespace MediaNet;

namespace MediaNet {

    enum struct HistoryStatus : uint8_t {
        // this is bit vector were bits indicate set, received, congested, lost
        none = 0,
        sent = 1,
        congested = 2+1, // ECN was set for this
        received = 4+1, // received but lost ACK
        ack  = 8+4+1, // got ACK and not congested
        lost = 0x10+1  // no ACK in time
    };

struct PacketUpstreamStatus {
    uint32_t seqNum;
    uint16_t sizeBits;

    uint32_t localSendTimeUs;
    uint32_t remoteReceiveTimeUs;
    uint32_t localAckTimeUs;

    uint32_t sendPhaseCount;

    HistoryStatus status;

    MediaNet::ShortName shortName;
};

struct PacketDownstreamStatus {
  uint32_t remoteSeqNum;
  uint16_t sizeBits;

  uint32_t remoteSendTimeUs;
  uint32_t localReceiveTimeUs;

  uint32_t sendPhaseCount;

  HistoryStatus status;
};

class Filter{
public:
    Filter( int64_t initialValue, int64_t gainUp1024s, int64_t gainDown1024s, bool initOnFirstValue=false );
    void add(int64_t v);
    void update() {};
    void reset() { initOnFirst=true; };
    [[nodiscard]] int64_t estimate() const;
private:
    int64_t value;
    const  int64_t gainUp;
    const  int64_t gainDown;
    bool initOnFirst;
};

class RateCtrl {
public:
  explicit RateCtrl(PipeInterface *pacerPipeRef);

  void sendPacket(uint32_t seqNum, uint32_t sendTimeUs, uint16_t sizeBits,
                  ShortName shortName);

  void recvPacket(uint32_t relaySeqNum, uint32_t remoteSendTimeUs,
                  uint32_t localRecvTimeUs, uint16_t sizeBits, bool congested );
  void recvAck(uint32_t seqNum, uint32_t remoteAckTimeUs,
               uint32_t localRecvAckTimeUs, bool congested, bool haveAck );


  [[nodiscard]] uint32_t getPhase() const;

  [[nodiscard]] uint64_t bwUpTarget() const;   // in bits per second
  [[nodiscard]] uint64_t bwDownTarget() const; // in bits per second

private:
  PipeInterface *pacerPipe;

  uint32_t upHistorySeqOffset;
  std::vector<PacketUpstreamStatus> upstreamHistory;

  uint32_t downHistorySeqOffset;
  std::vector<PacketDownstreamStatus> downstreamHistory;

  void updatePhase();
  static const uint32_t phaseTimeUs = 33333 * 2; // 0.5 frames at 30 fps
  static const uint32_t numPhasePerCycle = 5;

  void startNewPhase();
  uint32_t phaseCycleCount; // does *not* reset to zero with each new cycle

  void startNewCycle();
  std::chrono::steady_clock::time_point cycleStartTime;

  void cycleUpdateUpstreamTarget();
  uint64_t upstreamBitrateTarget;

  void cycleUpdateDownstreamTarget();
  uint64_t downstreamBitrateTarget;

  void calcPhaseAll();

  void calcPhaseMinRTT(  int start, int end );
  MediaNet::Filter filterMinRTT;

  void calcPhaseBigRTT(int start, int end );
  MediaNet::Filter filterBigRTT;

  void calcPhaseClockSkew(   int start, int end );
  MediaNet::Filter filterLowerBoundSkew;
  MediaNet::Filter filterUpperBoundSkew;

  void calcPhaseJitterUp(   int start, int end );
  MediaNet::Filter filterJitterUp;

  void calcPhaseJitterDown(  int start, int end  );
  MediaNet::Filter filterJitterDown;

  void calcPhaseLossRateUp(   int start, int end  );
  MediaNet::Filter filterLossRatePerMillionUp;

  void calcPhaseLossRateDown(   int start, int end );
  MediaNet::Filter filterLossRatePerMillionDown;

  void calcPhaseBitrateUp(  int start, int end );
  MediaNet::Filter filterBitrateUp;

  void calcPhaseBitrateDown(   int start, int end );
  MediaNet::Filter filterBitrateDown;
};

} // namespace MediaNet
