

#include <cassert>
#include <chrono>
#include <iostream>

#include "fecPipe.hh"
#include "packet.hh"

using namespace MediaNet;

FecPipe::FecPipe(PipeInterface *t) : PipeInterface(t) {}

bool FecPipe::send(std::unique_ptr<Packet> packet) {
  assert(packet);
  assert(downStream);

  std::chrono::steady_clock::time_point tp = std::chrono::steady_clock::now();
  std::chrono::steady_clock::duration dn = tp.time_since_epoch();
  uint32_t nowMs =
      (uint32_t)std::chrono::duration_cast<std::chrono::milliseconds>(dn)
          .count();

  if (packet->getFEC() ) {
    // TODO for packets with FEC enabled, save them and send them again in 10 ms

   {
      std::unique_ptr<Packet> fecPacket = packet->clone();
       fecPacket->setFEC(false);
      fecPacket->setReliable( false );
      fecPacket->setPriority(  0 );
      sendList.emplace_back(nowMs + 10, std::move(fecPacket));
    }

  {
      std::unique_ptr<Packet> fecPacket = packet->clone();
      fecPacket->setFEC(false);
      fecPacket->setReliable( false );
      fecPacket->setPriority(  0 );
      sendList.emplace_back(nowMs + 50, std::move(fecPacket));
    }
  }

  if (!sendList.empty()) {
    while (sendList.front().first <= nowMs) {
      std::unique_ptr<Packet> fecPacket = std::move(sendList.front().second);
      sendList.pop_front();
      downStream->send(move(fecPacket));
    }
  }

  return downStream->send(move(packet));
}

std::unique_ptr<Packet> FecPipe::recv() {

  assert(downStream);
  return downStream->recv();
}

FecPipe::~FecPipe() {
  while (!sendList.empty()) {
    std::unique_ptr<Packet> fecPacket = std::move(sendList.front().second);
    sendList.pop_front();
  }
}