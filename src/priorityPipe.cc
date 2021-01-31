

#include <cassert>
#include <iostream>

#include "packet.hh"
#include "priorityPipe.hh"

using namespace MediaNet;

PriorityPipe::PriorityPipe(PipeInterface *t) : PipeInterface(t) {}

bool PriorityPipe::send(std::unique_ptr<Packet> packet) {
  assert(downStream);

  // TODO - implement priority Q that rate controller can use

    {
        std::lock_guard<std::mutex> lock(sendQMutex);
        // std::clog << "+";
        sendQ.push(move(packet));

        // TODO - check Q not too deep
    }
}

std::unique_ptr<Packet> PriorityPipe::toDownstream() {
    if (sendQ.empty()) {
        return std::unique_ptr<Packet>(nullptr);
    }

    std::unique_ptr<Packet> packet;
    {
        std::lock_guard<std::mutex> lock(sendQMutex);
        packet = move(sendQ.front());
        sendQ.pop();
    }

    return packet;
}
