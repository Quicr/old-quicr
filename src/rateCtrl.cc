
#include <algorithm>
#include <cassert>
#include <iomanip>
#include <iostream>

#include "rateCtrl.hh"

using namespace MediaNet;

// windows has a max macro which breaks things
#ifdef max
#undef max
#endif

RateCtrl::RateCtrl(PipeInterface *pacerPipeRef)
    : pacerPipe(pacerPipeRef), upHistorySeqOffset(0), downHistorySeqOffset(0),
      phaseCycleCount(0), filterMinRTT(10 * 1000, 102, 1024, true),
      filterBigRTT(50 * 1000, 1024, 10, true),
      filterLowerBoundSkew(0, 1024, 1, true),
      filterUpperBoundSkew(0, 1, 1024, true),
      filterJitterUp(1, 1024, 102, true), filterJitterDown(1, 1024, 102, true),
      filterLossRatePerMillionUp(0, 102, 102),
      filterLossRatePerMillionDown(0, 102, 102),
      filterBitrateUp(1e6, 1024, 0, true), limitBitrateMinUp(0),
      limitBitrateMaxUp(100e9), filterBitrateDown(1e6, 1024, 0, true) {
  upstreamHistory.clear();
  upstreamHistory.reserve(5000); // TODO limit length of history

  downstreamHistory.clear();
  downstreamHistory.reserve(5000); // TODO limit length of history

  startNewCycle();
}

void RateCtrl::sendPacket(uint32_t seqNum, uint32_t sendTimeUs,
                          uint16_t sizeBits, ShortName shortName) {
  updatePhase();

  assert(sizeBits > 0);

  if (seqNum < upHistorySeqOffset) {
    return;
  }
  // TODO - shrink history if too large

  if (seqNum - upHistorySeqOffset >= upstreamHistory.size()) {
    assert(seqNum - upHistorySeqOffset - upstreamHistory.size() < 10000);

    upstreamHistory.resize(seqNum - upHistorySeqOffset + 1);
  }
  PacketUpstreamStatus &rec = upstreamHistory.at(seqNum - upHistorySeqOffset);

  rec.seqNum = seqNum;
  rec.sizeBits = sizeBits;

  rec.localSendTimeUs = sendTimeUs;
  rec.remoteReceiveTimeUs = 0;
  rec.localAckTimeUs = 0;

  rec.sendPhaseCount = phaseCycleCount;
  rec.status = HistoryStatus::sent;

  rec.shortName = shortName;
}

void RateCtrl::recvAck(uint32_t seqNum, uint32_t remoteAckTimeUs,
                       uint32_t localRecvAckTimeUs, bool congested,
                       bool haveAck) {
  updatePhase();

  if (seqNum < upHistorySeqOffset) {
    // TODO - log really old
    return;
  }

  // TODO - shrink down history size if too large

  if (seqNum - upHistorySeqOffset >= upstreamHistory.size()) {
    // TODO - alien packet, toss out
    return; // this happens when you get data from old buffer from previous
            // session
  }

  PacketUpstreamStatus &rec = upstreamHistory.at(seqNum - upHistorySeqOffset);

  if (rec.seqNum != seqNum) {
    // TODO - figure out how this happens
    return;
  }
  assert(rec.seqNum == seqNum);

  if (rec.status == HistoryStatus::sent) {
    pacerPipe->ack(rec.shortName);
  }

  if (haveAck) {
    rec.status = (congested) ? HistoryStatus::congested : HistoryStatus::ack;
    rec.remoteReceiveTimeUs = remoteAckTimeUs;
    rec.localAckTimeUs = localRecvAckTimeUs;
  } else {
    if (rec.status == HistoryStatus::sent) {
      rec.status =
          (congested) ? HistoryStatus::congested : HistoryStatus::received;
      rec.remoteReceiveTimeUs = remoteAckTimeUs;
      rec.localAckTimeUs = 0;
    }
  }
}

uint64_t RateCtrl::bwUpTarget() const // in bits per second
{
  assert(numPhasePerCycle >= 3);

  uint64_t target = upstreamBitrateTarget;

  int phase = phaseCycleCount % numPhasePerCycle;
  if (phase == 1) {
    target = (upstreamBitrateTarget * 5) / 4;
  }
  if (phase == 2) {
    target = (upstreamBitrateTarget * 3) / 4;
  }

  if (target > limitBitrateMaxUp) {
    target = limitBitrateMaxUp;
  }

  if (target < limitBitrateMinUp) {
    target = limitBitrateMinUp;
  }

  return target;
}

uint64_t RateCtrl::bwDownTarget() const // in bits per second
{
  assert(numPhasePerCycle >= 4);

  int phase = phaseCycleCount % numPhasePerCycle;

  if (phase == 2) {
    return (downstreamBitrateTarget * 5) / 4;
  }
  if (phase == 3) {
    return (downstreamBitrateTarget * 3) / 4;
  }

  return downstreamBitrateTarget;
}

void RateCtrl::updatePhase() {
  auto timePointNow = std::chrono::steady_clock::now();
  uint32_t cycleTimeUs =
      (uint32_t)std::chrono::duration_cast<std::chrono::microseconds>(
          timePointNow - cycleStartTime)
          .count();

  uint32_t newPhase = cycleTimeUs / phaseTimeUs;
  uint32_t oldPhase = phaseCycleCount % numPhasePerCycle;

  if (newPhase >= numPhasePerCycle) {
    startNewPhase();
    startNewCycle();
    phaseCycleCount++;
  } else if (newPhase > oldPhase) {
    startNewPhase();
    phaseCycleCount += (newPhase - oldPhase);
  }
}

void RateCtrl::startNewPhase() { calcPhaseAll(); }

void RateCtrl::startNewCycle() {

#if 0
  int64_t  estRTTUs = filterMinRTT.estimate();
  int64_t  estBigRTTUs = filterBigRTT.estimate();
  int64_t  upstreamBwEst = filterBitrateUp.estimate();
  int64_t  downstreamBwEst = filterBitrateDown.estimate();
  int64_t  upstreamLossRate = filterLossRatePerMillionUp.estimate();
  int64_t  downstreamLossRate = filterLossRatePerMillionDown.estimate();
  int64_t  jitterUpUs = filterJitterUp.estimate();
  int64_t  jitterDownUs = filterJitterDown.estimate();

  if (phaseCycleCount > 0) {
    std::clog << "Cycle"
              << " cycle: " << phaseCycleCount / numPhasePerCycle
              << " estRtt: " << float(estRTTUs) / 1e3 << " ms "
              << " bigRTT: " << float(estBigRTTUs) / 1e3 << " ms "
              << std::endl;
    std::clog << " Up   bitrate: " << float(upstreamBwEst) / 1e6 << " mbps "
              << " jitter:" << (float)jitterUpUs/1e3 << " ms "
              << " lossRate: " << (float)upstreamLossRate/10000.0 << " %"  << std::endl;
    std::clog << " Down bitrate: " << float(downstreamBwEst) / 1e6 << " mbps "
              << " jitter:" << (float)jitterDownUs/1e3 << " ms "
              << " lossRate: " << (float)downstreamLossRate/10000.0 << " %"  << std::endl;
  }
#endif

  cycleUpdateUpstreamTarget();
  cycleUpdateDownstreamTarget();

  // TODO - send the filters as stats
  assert(pacerPipe);
  pacerPipe->updateStat(PipeInterface::StatName::bigRTTms,
                        filterBigRTT.estimate() / 1000);
  pacerPipe->updateStat(PipeInterface::StatName::minRTTms,
                        filterMinRTT.estimate() / 1000); // keep after big

  pacerPipe->updateStat(PipeInterface::StatName::lossPerMillionUp,
                        filterLossRatePerMillionUp.estimate());
  pacerPipe->updateStat(PipeInterface::StatName::lossPerMillionDown,
                        filterLossRatePerMillionDown.estimate());

  pacerPipe->updateStat(PipeInterface::StatName::bitrateUp,
                        filterBitrateUp.estimate());
  pacerPipe->updateStat(PipeInterface::StatName::bitrateDown,
                        filterBitrateDown.estimate());

  pacerPipe->updateStat(PipeInterface::StatName::jitterUpMs,
                        filterJitterUp.estimate() / 1000);
  pacerPipe->updateStat(PipeInterface::StatName::jitterDownMs,
                        filterJitterDown.estimate() / 1000);

  cycleStartTime = std::chrono::steady_clock::now();

  filterBitrateUp.reset();
  filterBitrateDown.reset();

  filterLossRatePerMillionUp.reset();
  filterLossRatePerMillionDown.reset();

  filterJitterUp.reset();
  filterJitterUp.reset();
}

void RateCtrl::recvPacket(uint32_t relaySeqNum, uint32_t remoteSendTimeUs,
                          uint32_t localRecvTimeUs, uint16_t sizeBits,
                          bool congested) {
  updatePhase();

#if 0
  std::clog << "Got subData seq=" << relaySeqNum
            << " remoteTime(ms)=" << remoteSendTimeUs / 1000
            << " localTime(us)=" << localRecvTimeUs << " size=" << sizeBits
            << " phaseCycleCount=" << phaseCycleCount
            << std::endl;
#endif

  // TODO - need to auth this as attacker could cause bad shift and loose data

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

  // TODO - add info in upstream history
  assert(sizeBits > 0);

  if (relaySeqNum < downHistorySeqOffset) {
    return;
  }
  if (relaySeqNum - downHistorySeqOffset >= downstreamHistory.size()) {
    assert(relaySeqNum - downHistorySeqOffset - downstreamHistory.size() <
           10000);

    downstreamHistory.resize(relaySeqNum - downHistorySeqOffset + 1);
  }

  // TODO - shrink history buffer if too large

  PacketDownstreamStatus &rec =
      downstreamHistory.at(relaySeqNum - downHistorySeqOffset);

  rec.remoteSeqNum = relaySeqNum;
  rec.sizeBits = sizeBits;

  rec.remoteSendTimeUs = remoteSendTimeUs;
  rec.localReceiveTimeUs = localRecvTimeUs;

  rec.sendPhaseCount = phaseCycleCount;

  rec.status = congested ? HistoryStatus::congested : HistoryStatus::received;

  // TODO - think about how to send NACK
}

uint32_t RateCtrl::getPhase() const {
  return (phaseCycleCount % numPhasePerCycle);
}

Filter::Filter(int64_t initialValue, int64_t gainUp1024s, int64_t gainDown1024s,
               bool initOnFirstValue)
    : value(initialValue), gainUp(gainUp1024s), gainDown(gainDown1024s),
      initOnFirst(initOnFirstValue) {}

void Filter::add(int64_t v) {
  if (initOnFirst) {
    initOnFirst = false;
    value = v;
  } else {
    if (v > value) {
      value = (v * gainUp + value * (1024 - gainUp)) / 1024;
    } else {
      value = (v * gainDown + value * (1024 - gainDown)) / 1024;
    }
  }
}

int64_t Filter::estimate() const { return value; }

void RateCtrl::calcPhaseAll() {
  uint64_t bigRttUs = filterBigRTT.estimate();

  uint32_t phasesBack = (bigRttUs / phaseTimeUs) + 1;

  if (phasesBack >= phaseCycleCount) {
    return;
  }
  uint32_t phaseCycle = phaseCycleCount - phasesBack;

  assert(upstreamHistory.size() > 0);
  int histUpEnd = upstreamHistory.size();
  while (upstreamHistory.at(histUpEnd - 1).sendPhaseCount > phaseCycle) {
    histUpEnd--;
    if (histUpEnd == 0) {
      return;
    }
  }
  int histUpStart = histUpEnd;
  while (upstreamHistory.at(histUpStart - 1).sendPhaseCount == phaseCycle) {
    histUpStart--;
    if (histUpStart == 0) {
      break;
    }
  }

  int histDownEnd = downstreamHistory.size();
  while (histDownEnd > 0) {
    // step over missing records
    if (downstreamHistory.at(histDownEnd - 1).status !=
        HistoryStatus::received) {
      histDownEnd--;
      continue;
    }

    if (downstreamHistory.at(histDownEnd - 1).sendPhaseCount > phaseCycle) {
      histDownEnd--;
    } else {
      break;
    }
  }
  int histDownStart = histDownEnd;
  while (histDownStart > 0) {
    // step over missing records
    if (downstreamHistory.at(histDownStart - 1).status !=
        HistoryStatus::received) {
      histDownStart--;
      continue;
    }
    if (downstreamHistory.at(histDownStart - 1).sendPhaseCount == phaseCycle) {
      histDownStart--;
    } else {
      break;
    }
  }

  // std::clog << "histPhase Up Start=" << histUpStart << "  end=" << histUpEnd
  // << std::endl; std::clog << "histPhase Down Start=" << histDownStart << "
  // end=" << histDownEnd << std::endl;

  // std::clog << "Phase ----------------- " << std::endl;

  const int numPacketsBackForBigRTT = 1000;
  int histUpStartBig = std::max(0, histUpStart - numPacketsBackForBigRTT);

  calcPhaseBigRTT(histUpStartBig, histUpEnd);
  calcPhaseMinRTT(histUpStart, histUpEnd);
  calcPhaseClockSkew(histUpStart, histUpEnd);
  calcPhaseJitterUp(histUpStart, histUpEnd);
  calcPhaseLossRateUp(histUpStart, histUpEnd);
  calcPhaseBitrateUp(histUpStart, histUpEnd);

  // TODO - could have more aggressive value for histDownEnd based on jitter
  // down

  calcPhaseJitterDown(histDownStart, histDownEnd);
  calcPhaseLossRateDown(histDownStart, histDownEnd);
  calcPhaseBitrateDown(histDownStart, histDownEnd);

  // TODO - update all the filters
  filterMinRTT.update();
  filterBigRTT.update();
  filterLowerBoundSkew.update();
  filterUpperBoundSkew.update();
  filterJitterUp.update();
  filterJitterDown.update();
  filterLossRatePerMillionUp.update();
  filterLossRatePerMillionDown.update();
  filterBitrateUp.update();
  filterBitrateDown.update();
}

void RateCtrl::calcPhaseMinRTT(int start, int end) {
  bool noneFound = true;
  int64_t minRttUs;
  for (int i = start; i < end; i++) {
    if (upstreamHistory.at(i).status == HistoryStatus::ack) {

      int64_t rttUs = (int64_t)upstreamHistory.at(i).localAckTimeUs -
                      (int64_t)upstreamHistory.at(i).localSendTimeUs;

      if (noneFound) {
        noneFound = false;
        minRttUs = rttUs;
      } else {
        minRttUs = std::min(rttUs, minRttUs);
      }
    }
  }
  if (!noneFound) {
    filterMinRTT.add(minRttUs);
    // std::clog << " phase minRTT=" << minRttUs/1000 << " ms" << std::endl;
    // std::clog << " phase estMinRTT=" << filterMinRTT.estimate() / 1000 << "
    // ms"
    //          << std::endl;
  }
}

void RateCtrl::calcPhaseClockSkew(int start, int end) {
  bool noneFound = true;
  int64_t maxLowerBoundUs;
  int64_t minUpperBoundUs;

  for (int i = start; i < end; i++) {
    if (upstreamHistory.at(i).status == HistoryStatus::ack) {

#if 0
      std::clog << "phase "
        << "send="  << upstreamHistory.at(i).localSendTimeUs/1000
        << ",recv=" << upstreamHistory.at(i).remoteReceiveTimeUs/1000
        << ",ack="  << upstreamHistory.at(i).localAckTimeUs/1000
        << " ms" << std::endl;
#endif

      int64_t upperBoundUs =
          (int64_t)upstreamHistory.at(i).remoteReceiveTimeUs -
          (int64_t)upstreamHistory.at(i).localSendTimeUs;
      int64_t lowerBoundUs =
          (int64_t)upstreamHistory.at(i).remoteReceiveTimeUs -
          (int64_t)upstreamHistory.at(i).localAckTimeUs;

      if (noneFound) {
        noneFound = false;
        maxLowerBoundUs = lowerBoundUs;
        minUpperBoundUs = upperBoundUs;
      } else {
        maxLowerBoundUs = std::max(maxLowerBoundUs, lowerBoundUs);
        minUpperBoundUs = std::min(minUpperBoundUs, upperBoundUs);
      }
    }
  }
  if (!noneFound) {
    filterLowerBoundSkew.add(maxLowerBoundUs);
    filterUpperBoundSkew.add(minUpperBoundUs);
    // std::clog<<"phase minSkew="<<maxLowerBoundUs/1000<<",maxSkew=" <<
    // minUpperBoundUs/1000 << " ms" << std::endl; std::clog << "phase
    // minSkewEst=" << filterLowerBoundSkew.estimate()/1000 << " ms" <<
    // std::endl; std::clog << "phase maxSkewEst=" <<
    // filterUpperBoundSkew.estimate()/1000 << " ms" << std::endl;
#if 0
    std::clog << " phase skewEst="
      << (filterLowerBoundSkew.estimate() + filterUpperBoundSkew.estimate()) / 2 / 1000
        << " ms" << std::endl;
#endif
  }
}

void RateCtrl::calcPhaseJitterUp(int start, int end) {
  int64_t skewUpperUs = filterUpperBoundSkew.estimate();

  bool noneFound = true;
  int64_t maxJitterUs;
  for (int i = start; i < end; i++) {
    if (upstreamHistory.at(i).status == HistoryStatus::ack) {

      int64_t sentTimeUs = (int64_t)upstreamHistory.at(i).localSendTimeUs;
      int64_t recvTimeUs =
          (int64_t)upstreamHistory.at(i).remoteReceiveTimeUs - skewUpperUs;

      int64_t jitterUs = recvTimeUs - sentTimeUs;
      // std::clog << "jitter=" << jitterUs/1000 << " ms" << std::endl;

      if (noneFound) {
        noneFound = false;
        maxJitterUs = jitterUs;
      } else {

        maxJitterUs = std::max(maxJitterUs, jitterUs);
      }
    }
  }
  if (!noneFound) {
    filterJitterUp.add(maxJitterUs);
    // std::clog << " phase maxJitterUp=" << maxJitterUs/1000 << " ms" <<
    // std::endl;
#if 0
    std::clog << " phase estMaxJitterUp=" << filterJitterUp.estimate() / 1000
              << " ms" << std::endl;
#endif
  }
}

void RateCtrl::calcPhaseJitterDown(int start, int end) {
  int64_t skewLowerUs = filterLowerBoundSkew.estimate();

  bool noneFound = true;
  int64_t maxJitterDownUs;
  for (int i = start; i < end; i++) {
    if (downstreamHistory.at(i).status == HistoryStatus::received) {

      int64_t sentTimeUs =
          (int64_t)downstreamHistory.at(i).remoteSendTimeUs - skewLowerUs;
      int64_t recvTimeUs = (int64_t)downstreamHistory.at(i).localReceiveTimeUs;

      int64_t jitterDownUs = recvTimeUs - sentTimeUs;

      // std::clog << " phase jitterDown=" << jitterDownUs/1000 << " ms" <<
      // std::endl;

      if (noneFound) {
        noneFound = false;
        maxJitterDownUs = jitterDownUs;
      } else {
        maxJitterDownUs = std::max(maxJitterDownUs, jitterDownUs);
      }
    }
  }
  if (!noneFound) {
    filterJitterDown.add(maxJitterDownUs);
    // std::clog << " phase maxJitterDownUs=" << maxJitterDownUs/1000 << " ms"
    // << std::endl;
#if 0
    std::clog << " phase estMaxJitterDown="
              << filterJitterDown.estimate() / 1000 << " ms" << std::endl;
#endif
  }
}

void RateCtrl::calcPhaseBigRTT(int start, int end) {
  std::vector<int64_t> rttList;
  rttList.reserve(end - start);

  for (int i = start; i < end; i++) {
    if (upstreamHistory.at(i).status == HistoryStatus::ack) {

      int64_t rttUs = (int64_t)upstreamHistory.at(i).localAckTimeUs -
                      (int64_t)upstreamHistory.at(i).localSendTimeUs;

      rttList.push_back(rttUs);
    }
  }
  if (rttList.size() > 1) {
    std::sort(rttList.begin(), rttList.end());
    // TODO - there are way faster algorithm than sort of all of this every time

    int index = (rttList.size() - 1) * 98 /
                100; // TODO - most 98 percentile constant out
    int32_t bigRttUs = rttList.at(index);

    filterBigRTT.add(bigRttUs);
    // std::clog << " phase bigRtt=" << bigRttUs / 1000 << " ms" << std::endl;
#if 0
    std::clog << " phase estBigRtt=" << filterBigRTT.estimate() / 1000 << " ms"
              << std::endl;
#endif
  }
}

void RateCtrl::calcPhaseLossRateUp(int start, int end) {

  for (int i = start; i < end; i++) {
    if (upstreamHistory.at(i).status == HistoryStatus::sent) {
      upstreamHistory.at(i).status = HistoryStatus::lost;
    }
  }

  int64_t lostCount = 0;
  int64_t packetCount = 0;
  for (int i = start; i < end; i++) {
    packetCount++;
    if ((upstreamHistory.at(i).status == HistoryStatus::congested) ||
        (upstreamHistory.at(i).status == HistoryStatus::lost)) {
      lostCount++;
    }
  }

  if (packetCount > 0) {
    int64_t lossPerMillion = lostCount * 1000000 / packetCount;

    filterLossRatePerMillionUp.add(lossPerMillion);
    // std::clog << " phase lostCountUp=" << lostCount << " of " <<
    // packetCount<< std::endl; std::clog << " phase lossRateUp = " <<
    // (float)lossPerMillion/10000.0 << "% " << std::endl;
#if 0
    std::clog << " phase estLossRateUp = "
              << (float)filterLossRatePerMillionUp.estimate() / 10000.0 << "%"
              << std::endl;
#endif
  }
}

void RateCtrl::calcPhaseLossRateDown(int start, int end) {

  for (int i = start; i < end; i++) {
    if ((downstreamHistory.at(i).status != HistoryStatus::received) &&
        (downstreamHistory.at(i).status != HistoryStatus::congested)) {
      downstreamHistory.at(i).status = HistoryStatus::lost;
    }
  }

  int64_t lostCount = 0;
  int64_t packetCount = 0;
  for (int i = start; i < end; i++) {
    packetCount++;
    if ((downstreamHistory.at(i).status == HistoryStatus::congested) ||
        (downstreamHistory.at(i).status == HistoryStatus::lost)) {
      lostCount++;
    }
  }

  if (packetCount > 0) {
    int64_t lossPerMillion = lostCount * 1000000 / packetCount;

    filterLossRatePerMillionDown.add(lossPerMillion);
    // std::clog << " phase lostCountDown=" << lostCount << " of " <<
    // packetCount<< std::endl; std::clog << " phase lossRateDown = " <<
    // (float)lossPerMillion/10000.0 << "% " << std::endl;
#if 0
    std::clog << " phase estLossRateDown = "
              << (float)filterLossRatePerMillionDown.estimate() / 10000.0 << "%"
              << std::endl;
#endif
  }
}

void RateCtrl::calcPhaseBitrateUp(int start, int end) {

  for (int i = start; i < end; i++) {
    if (upstreamHistory.at(i).status == HistoryStatus::sent) {
      upstreamHistory.at(i).status = HistoryStatus::lost;
    }
  }

  int64_t bitCount = 0;
  int64_t bitLostCount = 0;
  for (int i = start; i < end; i++) {
    // std::clog << " acked = " << bool( upstreamHistory.at(i).status ==
    // HistoryStatus::ack ) << std::endl; std::clog << " revcd = " << bool(
    // upstreamHistory.at(i).status == HistoryStatus::received ) << std::endl;

    if ((upstreamHistory.at(i).status == HistoryStatus::received) ||
        (upstreamHistory.at(i).status == HistoryStatus::ack)) {
      bitCount += upstreamHistory.at(i).sizeBits;
      // std::clog << " bits = " << upstreamHistory.at(i).sizeBits << " bits "
      // << std::endl;
    } else {
      bitLostCount += upstreamHistory.at(i).sizeBits;
    }
  }

  if (bitCount > 0) {
    int64_t bitRate = bitCount * 1000000 / phaseTimeUs;
    int64_t bitLostRate = bitLostCount * 1000000 / phaseTimeUs;
    filterBitrateUp.add(bitRate);
#if 0
    std::clog << "------------------------" << std::endl;
    std::clog << " phase target bitrateUp = " << (float)bwUpTarget()/1.0e6 << " mbps " << std::endl;
    std::clog << " phase bitrateUp = " << (float)bitRate/1.0e6 << " mbps " << std::endl;
    std::clog << " phase bitrateLostUp = " << (float)bitLostRate/1.0e6 << " mbps " << std::endl;
    std::clog << " phase estBitRateUp = "<< (float)filterBitrateUp.estimate()/1e6 << " mbps " << std::endl;
#else
    (void)bitLostRate;
#endif
  }
}

void RateCtrl::calcPhaseBitrateDown(int start, int end) {

  for (int i = start; i < end; i++) {
    if ((downstreamHistory.at(i).status != HistoryStatus::received) &&
        (downstreamHistory.at(i).status != HistoryStatus::congested)) {
      downstreamHistory.at(i).status = HistoryStatus::lost;
    }
  }

  int64_t bitCount = 0;
  for (int i = start; i < end; i++) {
    if (downstreamHistory.at(i).status == HistoryStatus::received) {
      bitCount += downstreamHistory.at(i).sizeBits;
    }
  }

  if (bitCount > 0) {
    int64_t bitRate = bitCount * 1000000 / phaseTimeUs;
    filterBitrateDown.add(bitRate);

#if 0
    //std::clog << " phase bitrateDown = " << (float)bitRate/1.0e6 << " mbps " << std::endl;
    std::clog << " phase estBitRateDown = "<< (float)filterBitrateDown.estimate()/1e6 << " mbps " << std::endl;
#endif
  }
}

void RateCtrl::cycleUpdateUpstreamTarget() {

  int64_t prevTargetUp = upstreamBitrateTarget;
  int64_t bitrateUp = filterBitrateUp.estimate();
  int64_t lossRatePerMillionUp = filterLossRatePerMillionUp.estimate();

  if (lossRatePerMillionUp > 600 * 1000) {
    // when 60% packet loss, drop to min of half prev rate or what works
    upstreamBitrateTarget = std::min(prevTargetUp / 2, bitrateUp);
  } else {
    // TODO - consider packet size and delay info in decision

    // move to max bitrate that worked
    upstreamBitrateTarget =
        std::min(bitrateUp, prevTargetUp * 3 / 2); // TODO - mirror in download
  }
}

void RateCtrl::cycleUpdateDownstreamTarget() {

  int64_t prevTargetDown = downstreamBitrateTarget;
  int64_t bitrateDown = filterBitrateDown.estimate();
  int64_t lossRatePerMillionDown = filterLossRatePerMillionDown.estimate();

  if (lossRatePerMillionDown > 600 * 1000) {
    // when 60% packet loss, drop to min of half prev rate or what works
    downstreamBitrateTarget = std::min(prevTargetDown / 2, bitrateDown);
  } else {
    // TODO - consider packet size and delay info in decision

    // move to max bitrate that worked
    downstreamBitrateTarget = bitrateDown;
  }
}

void RateCtrl::overrideMtu(uint16_t mtu, uint32_t pps) {
  (void)mtu;
  (void)pps;
}

void RateCtrl::overrideRTT(uint16_t minRttMs, uint16_t bigRttMs) {
  filterMinRTT.override(minRttMs * 1000);
  filterBigRTT.override(bigRttMs * 1000);
}

void RateCtrl::overrideBitrateUp(uint64_t minBps, uint64_t startBps,
                                 uint64_t maxBps) {

  filterBitrateUp.override(startBps);
  upstreamBitrateTarget = startBps;

  limitBitrateMinUp = minBps;
  limitBitrateMaxUp = maxBps;
}
