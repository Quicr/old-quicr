#pragma once
#include <map>
#include <list>
#include "packet.hh"
#include "name.hh"

using Face = MediaNet::IpAddr;

struct SubscriberInfo {
	MediaNet::ShortName name;
	Face face{};
};


class Fib {
	public:
    [[maybe_unused]] virtual void addSubscription(const MediaNet::ShortName& name, SubscriberInfo subscriberInfo) = 0;
    [[maybe_unused]] virtual void removeSubscription(const MediaNet::ShortName& name, const SubscriberInfo& subscriberInfo) = 0;
		[[nodiscard]] virtual std::list<SubscriberInfo> lookupSubscription(const MediaNet::ShortName& name) const = 0;

    virtual ~Fib() = default;
};

