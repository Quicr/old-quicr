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
	  void addSubscription(const MediaNet::ShortName& name, SubscriberInfo& subscriberInfo);
	  void removeSubscription(const MediaNet::ShortName& name, const SubscriberInfo& subscriberInfo);
		std::list<SubscriberInfo> lookupSubscription(const MediaNet::ShortName& name) const;

	private:
	std::multimap<MediaNet::ShortName, SubscriberInfo> fibStore;
	std::map<MediaNet::ShortName, SubscriberInfo> subscribers;
};

