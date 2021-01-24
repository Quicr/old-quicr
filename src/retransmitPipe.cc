

#include <cassert>
#include <iostream>

#include "encode.hh"
#include "packet.hh"
#include "retransmitPipe.hh"

using namespace MediaNet;

RetransmitPipe::RetransmitPipe(PipeInterface *t) : PipeInterface(t) {}

bool RetransmitPipe::send(std::unique_ptr<Packet> packet) {
  assert(downStream);

  // TODO - for reliably packets, cache them and resend it no ack received for
  // them
  if ( packet->isReliable() ) {
      auto clone = packet->clone();
      packet->setReliable(false);
      
      auto p = std::pair<Packet::ShortName, std::unique_ptr<Packet>>(packet->shortName(), move(clone));
      auto ret = rtxList.insert(move(p));
      if (ret.second == false) {
          // element was already in map - not unique name, not good
          std::clog << "Wanring seding same name twice" << std::endl;
          return false;
      }
  }

  return downStream->send(move(packet));
}

std::unique_ptr<Packet> RetransmitPipe::recv() {
  assert(downStream);

  // TODO - watch for ACK to stop retransmission

  return downStream->recv();
}

void RetransmitPipe::ack(Packet::ShortName name) {
    PipeInterface::ack(name);

    //if ( name.resourceID != 1) return; // TODO REMOVE

    std::clog << "Ack of " << name.resourceID << "/" << name.senderID << "/" << (int)name.sourceID
        << "/t=" << name.mediaTime << "/" << (int)name.fragmentID << std::endl;

}
