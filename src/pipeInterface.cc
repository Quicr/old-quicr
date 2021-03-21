
#include <cassert>

#include "quicr/pipeInterface.hh"

using namespace MediaNet;

PipeInterface::PipeInterface(PipeInterface *nxtPipe)
    : nextPipe(nxtPipe), prevPipe(nullptr) {}

PipeInterface::~PipeInterface() {}

bool PipeInterface::start(const uint16_t port, const std::string& server,
                          PipeInterface *upStreamLink) {
  prevPipe = upStreamLink;
  if (nextPipe) {
    return nextPipe->start(port, server, this);
  }

  return true;
}

bool PipeInterface::ready() const {
  if (nextPipe) {
    return nextPipe->ready();
  }
  return true;
}

void PipeInterface::stop() {
  if (nextPipe) {
    nextPipe->stop();
  }
}

void PipeInterface::runUpdates(const std::chrono::time_point<std::chrono::steady_clock> &now) {
  if (nextPipe) {
    return nextPipe->runUpdates(now);
  }
}

bool PipeInterface::send(std::unique_ptr<Packet> packet) {
  if (nextPipe) {
    return nextPipe->send(move(packet));
  }
  return false;
}

std::unique_ptr<Packet> PipeInterface::recv() {
  if (nextPipe) {
    return nextPipe->recv();
  }
  return std::unique_ptr<Packet>(nullptr);
}

bool PipeInterface::fromDownstream(std::unique_ptr<Packet> packet) {
  assert(prevPipe);
  return prevPipe->fromDownstream(move(packet));
}

void PipeInterface::updateStat(PipeInterface::StatName stat, uint64_t value) {
  if (prevPipe) {
    prevPipe->updateStat(stat, value);
  }
}

void PipeInterface::ack(ShortName name) {
  if (prevPipe) {
    prevPipe->ack(name);
  }
}

void PipeInterface::updateRTT(uint16_t minRtMs, uint16_t bigRtMs) {
  if (nextPipe) {
    nextPipe->updateRTT(minRtMs, bigRtMs);
  }
}

std::unique_ptr<Packet> PipeInterface::toDownstream() {
  return std::unique_ptr<Packet>(nullptr);
}

void PipeInterface::updateMTU(uint16_t mtu, uint32_t pps) {
  if (nextPipe) {
    nextPipe->updateMTU(mtu, pps);
  }
}

void PipeInterface::updateBitrateUp(uint64_t minBps, uint64_t startBps,
                                    uint64_t maxBps) {
  if (nextPipe) {
    nextPipe->updateBitrateUp(minBps, startBps, maxBps);
  }
}
