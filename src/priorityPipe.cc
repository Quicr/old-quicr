

#include <cassert>
#include <iostream>

#include "priorityPipe.hh"
#include "quicr/packet.hh"

using namespace MediaNet;

PriorityPipe::PriorityPipe(PipeInterface* t)
  : PipeInterface(t)
  , mtu(1200)
{}

bool PriorityPipe::send(std::unique_ptr<Packet> packet) {
  assert(nextPipe);
  uint8_t priority = packet->getPriority();
  if (priority > maxPriority) {
    priority = maxPriority;
  }

  {
    std::lock_guard<std::mutex> lock(sendQMutex);
    // std::clog << "+";
    sendQarray[priority].push(move(packet));

    // TODO - check Q not too deep
    if (sendQarray[priority].size() > 1000) {
      sendQarray[priority]
        .pop(); // this is wrong, should kill oldest data - TODO
    }
  }

  return true;
}

std::unique_ptr<Packet>
PriorityPipe::toDownstream()
{
  std::lock_guard<std::mutex> lock(sendQMutex);

  std::unique_ptr<Packet> packet = std::unique_ptr<Packet>(nullptr);

  for (uint8_t i = maxPriority; i > 0; i--) {
    if (!sendQarray[i].empty()) {
      packet = move(sendQarray[i].front());
      sendQarray[i].pop();

      // TODO - if below the MTU , add some more data to the packet

      break;
    }
  }

  // TODO - look at MTU and compbine multiple packets

  return packet;
}

std::unique_ptr<Packet>
PriorityPipe::recv()
{
  std::unique_ptr<Packet> ret = std::unique_ptr<Packet>(nullptr);

  std::lock_guard<std::mutex> lock(recvQMutex);
  if (!recvQ.empty()) {
    ret = move(recvQ.front());
    recvQ.pop();

    // std::clog << "-";
  }

  return ret;
}

bool
PriorityPipe::fromDownstream(std::unique_ptr<Packet> packet)
{

  std::lock_guard<std::mutex> lock(recvQMutex);
  recvQ.push(move(packet));

  // TODO - check Q not too deep

  return true;
}

void
PriorityPipe::updateMTU(uint16_t val, uint32_t pps)
{
  mtu = val;

  PipeInterface::updateMTU(val, pps);
}
