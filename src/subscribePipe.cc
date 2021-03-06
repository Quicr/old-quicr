

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

  auto subReq = Subscribe{name};
	auto hdr = Packet::Header(PacketTag::headerData);
  packet << hdr;
  packet << subReq;

  std::cout << "Subscribe: " << packet->to_hex() << std::endl;
  packet->setFEC(true);
  packet->setReliable(true);

  downStream->send(move(packet));

  // TODO thread to re-send subscribe list every second

  return true;
}

std::unique_ptr<Packet> SubscribePipe::recv() {
  auto packet = PipeInterface::recv();

  if (packet) {
    // std::clog << "Sub recv: fullSize=" << packet->fullSize() << " size=" <<
    //packet->size() << std::endl;
    if (packet->fullSize() > 19 + QUICR_HEADER_SIZE_BYTES) {
      if (packet->buffer.at(19 + QUICR_HEADER_SIZE_BYTES - 1) == packetTagTrunc(PacketTag::shortName)) {

      	const auto idx = 19 + QUICR_HEADER_SIZE_BYTES - 1;
      	auto sn = packetTagTrunc(PacketTag::shortName);
      	auto& val = packet->buffer.at(idx);
        auto clone = packet->clone();
        clone->resize(19);
        ShortName name{};
        bool ok = (clone >> name);
        if (!ok) {
          // assert(0); // TODO - remove and log bad packet
          return std::unique_ptr<Packet>(nullptr);
        }
        packet->name = name;

        // std::clog << "Sub Recv: " << packet->shortName() << std::endl;
      }
    }
  }

  return packet;
}
