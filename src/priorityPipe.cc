

#include <cassert>
#include <iostream>

#include "packet.hh"
#include "priorityPipe.hh"

using namespace MediaNet;

PriorityPipe::PriorityPipe(PipeInterface *t): PipeInterface(t) ,mtu(1200) {}

bool PriorityPipe::send(std::unique_ptr<Packet> packet) {
  assert(downStream);

  // TODO - implement priority Q that rate controller can use

  {
      uint8_t priority = packet->getPriority();
      if ( priority > maxPriority ) {
          priority=maxPriority;
      }

    std::lock_guard<std::mutex> lock(sendQMutex);
    // std::clog << "+";
    sendQarray[priority].push(move(packet));

    // TODO - check Q not too deep
  }

  return true;
}

std::unique_ptr<Packet> PriorityPipe::toDownstream() {
    std::lock_guard<std::mutex> lock(sendQMutex);

    std::unique_ptr<Packet> packet = std::unique_ptr<Packet>(nullptr);

    for (uint8_t i = maxPriority; i > 0; i--) {
        if (!sendQarray[i].empty()) {
            packet = move(sendQarray[i].front());
            sendQarray[i].pop();
            break;
        }
    }

    // TODO - look at MTU and compbine multiple packets

  return packet;
}


std::unique_ptr<Packet> PriorityPipe::recv() {
    std::unique_ptr<Packet> ret = std::unique_ptr<Packet>(nullptr);

    std::lock_guard<std::mutex> lock(recvQMutex);
    if (!recvQ.empty()) {
        ret = move(recvQ.front());
        recvQ.pop();

        // std::clog << "-";
    }

    return ret;
}

bool PriorityPipe::fromDownstream(std::unique_ptr<Packet> packet) {

    std::lock_guard<std::mutex> lock(recvQMutex);
    recvQ.push(move(packet));

    // TODO - check Q not too deep

    return true;
}

void PriorityPipe::updateMTU(uint16_t val) {
    mtu = val;

    PipeInterface::updateMTU(val);
}
