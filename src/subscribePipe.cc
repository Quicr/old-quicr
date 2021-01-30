

#include <cassert>
#include <iostream>

#include "encode.hh"
#include "name.hh"
#include "packet.hh"
#include "subscribePipe.hh"

using namespace MediaNet;

SubscribePipe::SubscribePipe(PipeInterface *t) : PipeInterface(t) {}

bool SubscribePipe::subscribe(const ShortName &name) {

  subscribeList.push_back(name);

  // send subscribe packet
  auto packet = std::make_unique<Packet>();
  packet->reserve(100); // TODO tune
  packet << PacketTag::headerMagicData;
  packet << name;
  packet << PacketTag::shortName;

  packet->enableFEC(true);
  packet->setReliable(true);

  downStream->send(move(packet));

  // TODO thread to re-send subscribe list every second

  return true;
}

std::unique_ptr<Packet> SubscribePipe::recv() {
  auto packet = PipeInterface::recv();

  if (packet) {
    // std::clog << "Sub recv: fullSize=" << packet->fullSize() << " size=" <<
    // packet->size() << std::endl;
    if (packet->fullSize() > 19) {
      if (packet->buffer.at(19) == packetTagTrunc(PacketTag::shortName)) {

        auto clone = packet->clone();
        clone->resize(20);
        ShortName name;
        bool ok = (clone >> name);
        if (!ok) {
          assert(0); // TODO - remove and log bad packet
          return std::unique_ptr<Packet>(nullptr);
        }
        packet->name = name;

        // std::clog << "Sub Recv: " << packet->shortName() << std::endl;
      }
    }
  }

  return packet;
}
