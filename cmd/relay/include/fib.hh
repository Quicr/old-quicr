#pragma once
#include "quicr/packet.hh"
#include "quicr/shortName.hh"
#include <list>
#include <map>

using Face = MediaNet::IpAddr;

struct SubscriberInfo {
  MediaNet::ShortName name;
  Face face{};
  uint32_t relaySeqNum;
};

class Fib {
public:
  [[maybe_unused]] virtual void
  addSubscription(const MediaNet::ShortName &name,
                  SubscriberInfo subscriberInfo) = 0;
  [[maybe_unused]] virtual void
  removeSubscription(const MediaNet::ShortName &name,
                     const SubscriberInfo &subscriberInfo) = 0;
  [[nodiscard]] virtual std::list<SubscriberInfo>
  lookupSubscription(const MediaNet::ShortName &name) const = 0;

  virtual ~Fib() = default;
};
