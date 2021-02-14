#pragma once
#include <map>
#include <list>
#include "packet.hh"
#include "name.hh"

using Face = MediaNet::IpAddr;

struct SubscriberInfo {
	uint32_t id;
	MediaNet::ShortName name;
	Face face;
};


struct Fib {
	public:
	  virtual void addSubscription(const MediaNet::ShortName& name, SubscriberInfo subscriberInfo) = 0;
	  virtual void removeSubscription(const MediaNet::ShortName& name, const SubscriberInfo& subscriberInfo) = 0;
		virtual std::list<SubscriberInfo> lookupSubscription(const MediaNet::ShortName& name) const = 0;
};

