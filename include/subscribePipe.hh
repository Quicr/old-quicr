#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include "packet.hh"
#include "pipeInterface.hh"

namespace MediaNet {

class SubscribePipe : public PipeInterface {
public:
  explicit SubscribePipe(PipeInterface *t);

    bool subscribe( Packet::ShortName );

private:
    std::vector<Packet::ShortName> subscribeList;
};

} // namespace MediaNet
