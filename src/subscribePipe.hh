#pragma once

#include <cstdint>
#include <memory>
#include <map>

#include "pipeInterface.hh"
#include "quicr/packet.hh"

namespace MediaNet {

class SubscribePipe : public PipeInterface {
public:
  explicit SubscribePipe(PipeInterface *t);

  bool subscribe(const ShortName &name);
  std::unique_ptr<Packet> recv() override;

private:
  std::map<MediaNet::ShortName, bool> subscribeList;
};

} // namespace MediaNet
