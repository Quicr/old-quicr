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

    bool subscribe( ShortName name );

private:
    std::vector<MediaNet::ShortName> subscribeList;
};

} // namespace MediaNet
