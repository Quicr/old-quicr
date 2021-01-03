#pragma once

#include <cstdint>
#include <memory>

#include "packet.hh"
#include "pipeInterface.hh"

namespace MediaNet {

class SubscribePipe : public PipeInterface {
public:
  SubscribePipe(PipeInterface *t);

private:
};

} // namespace MediaNet
