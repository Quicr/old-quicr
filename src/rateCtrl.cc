
#include <cassert>
#include <iomanip>
#include <iostream>
#include <limits>

#include "rateCtrl.hh"

using namespace MediaNet;

// windows has a max macro which breaks things
#ifdef max
#undef max
#endif

RateCtrl::RateCtrl()
    : upHistorySeqOffset(0), downHistorySeqOffset(0), phase(0),
      bitsSentThisPhase(0), cycleCount(0),
      minCycleRTTUs(std::numeric_limits<uint32_t>::max()), maxCycleAckTimeUs(0),
      estRTTUs(100 * 1000), estAckTimeUs(100 * 1000), cycleRelayTimeOffsetUs(0),
      estRelayTimeOffsetUs(0), upstreamPacketLossRate(0), upstreamCycleMaxBw(0),
      upstreamBwEst(1000 * 1000), downstreamPacketLossRate(0.0),
      downstreamCycleMaxBw(0), downstreamBwEst(1000 * 1000) {
  upstreamHistory.clear();
  upstreamHistory.reserve(100); // TODO limit length of history

  downstreamHistory.clear();
  downstreamHistory.reserve(100); // TODO limit length of history

  startNewCycle();
  cycleCount = 0;
}

void RateCtrl::sendPacket(uint32_t seqNum, uint32_t sendTimeUs,
                          uint16_t sizeBits) {
  updatePhase();

  assert(sizeBits > 0);

  if (seqNum < upHistorySeqOffset) {
    return;
  }
  if (seqNum - upHistorySeqOffset >= upstreamHistory.size()) {
    assert(seqNum - upHistorySeqOffset - upstreamHistory.size() < 10000);

    upstreamHistory.resize(seqNum - upHistorySeqOffset + 1);
  }
  PacketUpstreamStatus &rec = upstreamHistory.at(seqNum - upHistorySeqOffset);

  rec.seqNum = seqNum;
  rec.sendTimeUs = sendTimeUs;
  rec.sizeBits = sizeBits;
  rec.sendPhaseCount = cycleCount * numPhasePerCycle + phase;
  rec.notLost = false;

  bitsSentThisPhase += sizeBits;
}

void RateCtrl::recvAck(uint32_t seqNum, uint32_t remoteAckTimeUs,
                       uint32_t localRecvAckTimeUs) {
  if (seqNum < upHistorySeqOffset) {
    return;
  }

  if (seqNum - upHistorySeqOffset >= upstreamHistory.size()) {
    return; // this happens when you old buffer from previous session
  }

  PacketUpstreamStatus &rec = upstreamHistory.at(seqNum - upHistorySeqOffset);

  if (rec.notLost) {
    return;
  }

  if (rec.seqNum != seqNum) {
    // TODO - figure out how this happens
    return;
  }
  rec.notLost = true;
  assert(rec.seqNum == seqNum);

  rec.remoteAckTimeUs = remoteAckTimeUs;
  rec.localRecvAckTimeUs = localRecvAckTimeUs;

  uint32_t rtt = rec.localRecvAckTimeUs - rec.sendTimeUs;
  int32_t timeOffset =
      (rec.localRecvAckTimeUs + rec.sendTimeUs) / 2 - rec.remoteAckTimeUs;
  updateRTT(rtt, timeOffset);

  if (seqNum - upHistorySeqOffset < 1) {
    return;
  }

  PacketUpstreamStatus &prev =
      upstreamHistory.at(seqNum - upHistorySeqOffset - 1);
  if (!prev.notLost) {
    return;
  }

#if 0
    std::cout << " seq:" << rec.seqNum;
    std::cout << " dtx:" << float(rec.sendTimeUs - prev.sendTimeUs) / 1000.0;
    std::cout << " drx:"
              << float(rec.remoteAckTimeUs - prev.remoteAckTimeUs) / 1000.0;
    if ((rec.remoteAckTimeUs != 0) & (prev.remoteAckTimeUs != 0)) {
      std::cout << " rtt:"
                << float(rec.localRecvAckTimeUs - rec.sendTimeUs) / 1000.0;
    }
    std::cout << std::endl;
#endif
}

uint32_t RateCtrl::rttEstUs() const { return estRTTUs; }

uint64_t RateCtrl::bwUpEst() const // in bits per second
{
  return upstreamBwEst;
}

uint64_t RateCtrl::bwDownEst() const // in bits per second
{
  return downstreamBwEst;
}

uint64_t RateCtrl::bwUpTarget() const // in bits per second
{
#if 0
  // TODO remove
  upstreamBwEst = 1 * 1000*1000;
  return upstreamBwEst;
#endif

  if (phase == 1) {
    return (upstreamBwEst * 5) / 4;
  }
  if (phase == 2) {
    return (upstreamBwEst * 3) / 4;
  }

  return (upstreamBwEst * 9) / 10;
}

uint64_t RateCtrl::bwDownTarget() const // in bits per second
{
  assert(numPhasePerCycle > 4);
#if 0
  // TODO remove
  downstreamBwEst = 1 * 1000*1000;
  return downstreamBwEst;
#endif

  if (phase == 3) {
    return (downstreamBwEst * 5) / 4;
  }
  if (phase == 4) {
    return (downstreamBwEst * 3) / 4;
  }

  return (downstreamBwEst * 9) / 10;
}

void RateCtrl::updatePhase() {
  auto timePointNow = std::chrono::steady_clock::now();
  uint32_t cycleTimeUs =
      (uint32_t)std::chrono::duration_cast<std::chrono::microseconds>(
          timePointNow - cycleStartTime)
          .count();

  uint32_t newPhase = cycleTimeUs / phaseTimeUs;
  if (newPhase >= numPhasePerCycle) {
    startNewPhase();
    startNewCycle();
    phase = 0;
  } else if (phase != newPhase) {
    startNewPhase();
    phase = newPhase;
  }
}

void RateCtrl::startNewPhase() {
  estUpstreamBw();
  estDownstreamBw();

  bitsSentThisPhase = 0;
}

void RateCtrl::startNewCycle() {
  if (minCycleRTTUs < std::numeric_limits<uint32_t>::max()) {
    estRTTUs = minCycleRTTUs; // TODO - filter
  }
  if (minCycleRTTUs > 0) {
    estAckTimeUs = maxCycleAckTimeUs; // TODO - filter
  }

  if (cycleRelayTimeOffsetUs > 0) {
    estRelayTimeOffsetUs = cycleRelayTimeOffsetUs;
  }

  if (upstreamCycleMaxBw > 0.0) {
    upstreamBwEst = upstreamCycleMaxBw; // TODO - filter
  }

#if 1
  if (cycleCount > 0) {
    std::clog << "Cycle"
              << " cycle:" << cycleCount
              << " estRtt(ms):" << float(estRTTUs) / 1e3
              << " maxAck(ms):" << float(estAckTimeUs) / 1e3
              << " upstreamBwEst(mbps):" << float(upstreamBwEst) / 1e6
              << " downstreamBwEst(mbps):" << float(downstreamBwEst) / 1e6
              << " relayTimeOffset(ms):" << float(estRelayTimeOffsetUs) / 1e3
              << std::endl;
  }
#endif

  minCycleRTTUs = std::numeric_limits<uint32_t>::max();
  maxCycleAckTimeUs = 0;

  cycleRelayTimeOffsetUs = 0;
  upstreamCycleMaxBw = 0;
  downstreamCycleMaxBw = 0;

  cycleStartTime = std::chrono::steady_clock::now();
  cycleCount++;
}

void RateCtrl::updateRTT(uint32_t valUs, int32_t newRemoteTimeOffsetUs) {
  if (valUs < minCycleRTTUs) {
    minCycleRTTUs = valUs;
    cycleRelayTimeOffsetUs = newRemoteTimeOffsetUs;
  }
  if (valUs > maxCycleAckTimeUs) {
    maxCycleAckTimeUs = valUs;
  }
}

void RateCtrl::estUpstreamBw() {
  if (upstreamHistory.size() <= 2) {
    return; // no buffer yet
  }

  const uint32_t phaseBack = 5; // TODO FIX

  if (upstreamHistory.back().sendPhaseCount < phaseBack) {
    return;
  }

  uint32_t prevPhaseCount = upstreamHistory.back().sendPhaseCount - phaseBack;

  /* TODO - check that the prevPhase is further back in time that the
   * ack time estimate */

  int lost = 0;
  int notLost = 0;
  uint32_t firstTime = 0;
  uint32_t lastTime = 0;
  uint32_t bitsReceived = 0;

  auto i = upstreamHistory.end();
  while (i != upstreamHistory.begin()) {
    i--;

    if (i->sendPhaseCount < prevPhaseCount) {
      break;
    }

    if (i->sendPhaseCount > prevPhaseCount) {
      if (i->notLost) {
        lastTime = i->remoteAckTimeUs;
      }
      continue;
    }

    if (!(i->notLost)) {
      lost++;
      continue;
    }

    // std::log << " seq=" <<  i->seqNum
    //          << " sph=" << i->sendPhaseCount
    //          << " lost=" << i->lost
    //          << std::endl;

    notLost++;

    firstTime = i->remoteAckTimeUs;
    bitsReceived += i->sizeBits;
  }

  if (lost + notLost == 0) {
    return;
  }

  float packetLossRate = float(lost) / float(lost + notLost);
  upstreamPacketLossRate = packetLossRate; // TODO filter

  float bps = 0.0;
  if (notLost >= 2) { // TODO - should we require more that two packets ???
    uint32_t dTimeUs =
        lastTime -
        firstTime; // TODO - do anything special if too stop together ????
    bps = float(bitsReceived) * float(1e6 /*convert to seconds*/) /
          float(dTimeUs);
  }

#if 0
  std::clog << "Up - Phase" << phase << std::setprecision(1) << std::fixed
            << " loss:" << packetLossRate * 100.0 << " % "
            << std::setprecision(3) << " txRate:"
            << float(bitsSentThisPhase) / float(phaseTimeUs); // in mbps

  if (bps > 0.0) {
    std::clog << " bw:" << std::setprecision(3) << std::fixed << bps / 1e6
              << " mbps ";
  }

  std::clog << std::defaultfloat << std::setprecision(6) << std::endl;
#endif

  updateUpstreamBwFilter(bps, upstreamPacketLossRate, 0.0 /*TODO*/);
}

void RateCtrl::recvPacket(uint32_t relaySeqNum, uint32_t remoteSendTimeMs,
                          uint32_t localRecvTimeUs, uint16_t sizeBits) {
  updatePhase();

#if 0
  std::clog << "Got subData seq=" << relaySeqNum
            << " remoteTime(ms)=" << remoteSendTimeMs
            << " localTime(us)=" << localRecvTimeUs << " size=" << sizeBits
            << " spc=" << cycleCount * numPhasePerCycle + phase
            << std::endl;
#endif

  uint32_t absSeqDiff = (relaySeqNum > downHistorySeqOffset)
                            ? (relaySeqNum - downHistorySeqOffset)
                            : (downHistorySeqOffset - relaySeqNum);

  if (absSeqDiff > 5000) {
    // std::clog << "reset relay seq number history" << std::endl;
    downHistorySeqOffset = relaySeqNum;
    downstreamHistory.clear();
    downstreamHistory.reserve(10000);
  }

  // TODO shift history and change downHistorySeqOffset as history grows

  // TODO - update info in upstream history
  assert(sizeBits > 0);

  if (relaySeqNum < downHistorySeqOffset) {
    return;
  }
  if (relaySeqNum - downHistorySeqOffset >= downstreamHistory.size()) {
    assert(relaySeqNum - downHistorySeqOffset - downstreamHistory.size() <
           10000);

    downstreamHistory.resize(relaySeqNum - downHistorySeqOffset + 1);
  }
  PacketDownstreamStatus &rec =
      downstreamHistory.at(relaySeqNum - downHistorySeqOffset);

  rec.relaySeqNum = relaySeqNum;
  rec.receiveTimeUs = localRecvTimeUs;
  rec.remoteSendTimeUs = remoteSendTimeMs;
  rec.sizeBits = sizeBits;
  rec.sendPhaseCount = cycleCount * numPhasePerCycle + phase;
  rec.notLost = true;

  // TODO - think about where NACK get sent
}

void RateCtrl::estDownstreamBw() {

  if (downstreamHistory.size() <= 2) {
    return; // no buffer yet
  }

  const uint32_t phaseBack = 2; // TODO FIX

  if (downstreamHistory.back().sendPhaseCount < phaseBack) {
    return;
  }

  uint32_t prevPhaseCount = downstreamHistory.back().sendPhaseCount - phaseBack;

  /* TODO - check that the prevPhase is further back in time that the
   * downstream jitter time estimate */

  int lostCount = 0;
  int notLostCount = 0;
  uint32_t firstTime = 0;
  uint32_t lastTime = 0;
  uint32_t bitsReceived = 0;
  bool inRange = false;

  auto i = downstreamHistory.end();
  while (i != downstreamHistory.begin()) {
    i--;

    if (i->notLost) {
      // ignore lost stuff as sendPhaseCount is not filled

      if (i->sendPhaseCount < prevPhaseCount) {
        break;
      }

      if (i->sendPhaseCount > prevPhaseCount) {
        if (i->notLost) {
          lastTime = i->receiveTimeUs;
        }

        continue;
      }

      inRange = true;
    }

    if (!inRange) {
      continue;
    }

    if (!i->notLost) {
      // std::clog << "Lost  seq=" << i->relaySeqNum
      //          << " sph=" << i->sendPhaseCount << std::endl;

      lostCount++;

      continue;
    }

    // std::clog << "Counting  seq=" << i->relaySeqNum
    //          << " sph=" << i->sendPhaseCount << std::endl;

    notLostCount++;

    firstTime = i->receiveTimeUs;
    bitsReceived += i->sizeBits;
  }

  if (lostCount + notLostCount == 0) {
    return;
  }

  float packetLossRate = float(lostCount) / float(lostCount + notLostCount);
  downstreamPacketLossRate = packetLossRate; // TODO filter

  float bps = 0.0;
  if (notLostCount >= 2) { // TODO - should we require more that two packets ???
    uint32_t dTimeUs =
        lastTime -
        firstTime; // TODO - do anything special if too stop together ????
    bps = float(bitsReceived) * float(1e6 /*convert to seconds*/) /
          float(dTimeUs);
  }

#if 0
  std::clog << "Dn - Phase" << phase << std::setprecision(1) << std::fixed
            << " loss:" << packetLossRate * 100.0 << " % "
            << std::setprecision(3);

  if (bps > 0.0) {
    std::clog << " bw:" << std::setprecision(3) << std::fixed << bps / 1e6
              << " mbps ";
  }

  std::clog << std::defaultfloat << std::setprecision(6) << std::endl;
#endif

  updateDownstreamBwFilter(bps, downstreamPacketLossRate, 0.0 /*TODO*/);
}

void RateCtrl::updateDownstreamBwFilter(float bps, float lossRate,
                                        float /* delayMs */) {

  if (lossRate > 0.5F) {
    if (downstreamBwEst * 0.5F > bps) {
      downstreamBwEst = downstreamBwEst * 0.5F;
    }
  }

  // TODO filter
  if (bps > downstreamBwEst) {
    downstreamBwEst = bps;
  }

  if (bps > downstreamCycleMaxBw) {
    downstreamCycleMaxBw = bps;
  }
}

void RateCtrl::updateUpstreamBwFilter(float bps, float lossRate,
                                      float /* delayMs */) {

  if (lossRate > 0.5F) {
    if (upstreamBwEst * 0.5F > bps) {
      upstreamBwEst = upstreamBwEst * 0.5F;
    }
  }

  // TODO filter
  if (bps > upstreamBwEst) {
    upstreamBwEst = bps;
  }

  if (bps > upstreamCycleMaxBw) {
    upstreamCycleMaxBw = bps;
  }
}
