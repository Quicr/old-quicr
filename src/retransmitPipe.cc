

#include <cassert>
#include <iostream>

#include "quicr/encode.hh"
#include "quicr/packet.hh"
#include "quicr/retransmitPipe.hh"

using namespace MediaNet;

RetransmitPipe::RetransmitPipe(PipeInterface *t)
    : PipeInterface(t), maxActTime(0), minRtt(100), bigRtt(200) {}

bool RetransmitPipe::send(std::unique_ptr<Packet> packet) {
  assert(nextPipe);

  // for reliably packets, cache them and resend if no ack received
  if (packet->isReliable()) {
    auto clone = packet->clone();
    assert(clone);
    packet->setReliable(false);

    auto p = std::pair<MediaNet::ShortName, std::unique_ptr<Packet>>(
        packet->shortName(), move(clone));

    std::lock_guard<std::mutex> lock(rtxListMutex);
    auto ret = rtxList.insert(move(p));
    if (!ret.second) {
      // element was already in map - not unique name, not good
      std::clog << "Warning sending same name twice" << std::endl;
      return false;
    }
  }

  return nextPipe->send(move(packet));
}

std::unique_ptr<Packet> RetransmitPipe::recv() {
  assert(nextPipe);

  return nextPipe->recv();
}

void RetransmitPipe::ack(ShortName name) {
  PipeInterface::ack(name);
  std::lock_guard<std::mutex> lock(rtxListMutex);

  rtxList.erase(name);

  if (name.mediaTime > maxActTime) {
    maxActTime = name.mediaTime;
  }

#if 0
    std::clog << "Ack of " << name.resourceID << "/" << name.senderID << "/" << (int) name.sourceID
              << "/t=" << name.mediaTime << "/" << (int) name.fragmentID
              << "  rtxList_size= " << rtxList.size() << std::endl;

    for (auto it = rtxList.cbegin(); it != rtxList.cend(); ++it) {
        std::clog << " [" << (*it).first.mediaTime << ']';
    }
    std::clog << std::endl;
#endif

  for (auto it = rtxList.begin(); it != rtxList.end();) {
    // TODO - fix 15 ms static timing here with maxRTT - minRTT
    assert(bigRtt >= minRtt);

    if ((*it).first.mediaTime < maxActTime - (bigRtt - minRtt)) {
      // resend this one
      nextPipe->send(move((*it).second));
      it = rtxList.erase(it);
    } else {
      it++;
    }
  }
}

void RetransmitPipe::updateRTT(uint16_t minRttMs, uint16_t bigRttMs) {
  PipeInterface::updateRTT(minRttMs, bigRttMs);

  bigRtt = bigRttMs;
  minRtt = minRttMs;
}
