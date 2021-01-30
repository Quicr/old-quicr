

#include <cassert>
#include <iostream>

#include "encode.hh"
#include "packet.hh"
#include "subscribePipe.hh"

using namespace MediaNet;

SubscribePipe::SubscribePipe(PipeInterface *t) : PipeInterface(t) {}

bool SubscribePipe::subscribe(ShortName name) {

    subscribeList.push_back( name );

    // send subscribe packet
    auto packet = std::make_unique<Packet>();
    packet->reserve(100 ); // TODO tune
    packet << PacketTag::headerMagicData;
    packet << name;
    packet << PacketTag::shortName;

    packet->enableFEC(true);
    packet->setReliable( true);

    downStream->send(move(packet));

    // TODO thread to re-send subscribe list every second

    return true;
}
