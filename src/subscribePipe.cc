

#include <cassert>
#include <iostream>

#include "encode.hh"
#include "name.hh"
#include "packet.hh"
#include "subscribePipe.hh"

using namespace MediaNet;

SubscribePipe::SubscribePipe(PipeInterface *t) : PipeInterface(t) {}

bool SubscribePipe::subscribe(const ShortName &name) {

  subscribeList.emplace(name, true);

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
    if(nextTag(packet) == PacketTag::shortName) {
		NamedDataChunk name{};
		bool ok = (packet >> name);
		if (!ok) {
			// assert(0); // TODO - remove and log bad packet
			return nullptr;
		}
		packet->name = name.shortName;
		packet << name;
	}
  }

  return packet;
}
