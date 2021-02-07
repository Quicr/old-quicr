

#include <cassert>
#include <iostream>
#include <chrono>

#include "crazyBitPipe.hh"
#include "packet.hh"

using namespace MediaNet;

CrazyBitPipe::CrazyBitPipe(PipeInterface *t)
    : PipeInterface(t), rttMs(100), spinBitVal(false), lastSpinTimeMs(0) {
  std::chrono::steady_clock::time_point tp = std::chrono::steady_clock::now();
  std::chrono::steady_clock::duration dn = tp.time_since_epoch();
  lastSpinTimeMs =
      (uint32_t)std::chrono::duration_cast<std::chrono::milliseconds>(dn)
          .count();
}

bool CrazyBitPipe::send(std::unique_ptr<Packet> packet) {
  std::chrono::steady_clock::time_point tp = std::chrono::steady_clock::now();
  std::chrono::steady_clock::duration dn = tp.time_since_epoch();
  uint32_t nowMs =
      (uint32_t)std::chrono::duration_cast<std::chrono::milliseconds>(dn)
          .count();

  if (nowMs > lastSpinTimeMs + rttMs) {
    spinBitVal = !spinBitVal;
    lastSpinTimeMs = nowMs;
    // std::clog << "flip spin bit to " << (spinBitVal?1:0) << std::endl;
  }

  if (spinBitVal) {
    assert(packet->fullSize() >= 1);
    packet->fullData() |= 0x01; // set the spin bit
  }

  assert(downStream);
  return downStream->send(move(packet));
}

std::unique_ptr<Packet> CrazyBitPipe::recv() {
  assert(downStream);
  std::unique_ptr<Packet> packet = downStream->recv();
  if (!packet) {
    return packet;
  }

  // clear the spin bit in first byte of incoming packet
  assert(packet->fullSize() >= 1);
  packet->fullData() &= 0xFE;

  return packet;
}

void CrazyBitPipe::updateRTT(uint16_t minRttMs, uint16_t maxRttMs) {
  PipeInterface::updateRTT(minRttMs, maxRttMs);

  rttMs = minRttMs;

  // std::clog << "Spin bit RTT set to " << rttMs << " ms" << std::endl;
}
