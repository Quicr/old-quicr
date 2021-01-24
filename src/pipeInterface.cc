
#include <cassert>

#include "pipeInterface.hh"

using namespace MediaNet;

PipeInterface::PipeInterface(PipeInterface *t)
    : downStream(t), upStream(nullptr) {}

PipeInterface::~PipeInterface() {}

bool PipeInterface::start(const uint16_t port, const std::string server,
                          PipeInterface *upStreamLink) {
  assert(downStream);
  // assert(upStreamLink);
  upStream = upStreamLink;
  return downStream->start(port, server, this);
}

bool PipeInterface::ready() const {
  assert(downStream);
  return downStream->ready();
}

void PipeInterface::stop() {
  assert(downStream);
  downStream->stop();
}

bool PipeInterface::send(std::unique_ptr<Packet> packet) {
  assert(downStream);
  return downStream->send(move(packet));
}

std::unique_ptr<Packet> PipeInterface::recv() {
  assert(downStream);
  return downStream->recv();
}

bool PipeInterface::fromDownstream(std::unique_ptr<Packet> packet) {
  assert(upStream);
  return upStream->fromDownstream(move(packet));
}

void PipeInterface::updateStat(PipeInterface::StatName stat, uint64_t value) {
    assert(upStream);
    upStream->updateStat( stat, value );
}

void PipeInterface::ack(Packet::ShortName name) {
    assert(upStream);
    upStream->ack( name );
}

void PipeInterface::updateRTT(uint64_t rttMs) {
    assert(downStream);
    downStream->updateRTT(rttMs);
}

