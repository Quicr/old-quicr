#include "../include/fib.hh"
#include <iostream>

void Fib::addSubscription(const MediaNet::ShortName& name, SubscriberInfo& subscriberInfo) {
	fibStore.insert(std::make_pair(name, std::move(subscriberInfo)));
	std::clog << name << "-has-" << fibStore.count(name) << "\n";
}

void Fib::removeSubscription(const MediaNet::ShortName& name, const SubscriberInfo& subscriberInfo) {
	if(!fibStore.count(name)) {
		return;
	}

	auto entries = fibStore.equal_range(name);
	auto it = std::find_if(entries.first, entries.second,
												[&subscriberInfo](auto const& entry){
		return entry.second.id == subscriberInfo.id;
	});

	fibStore.erase(it);
}

std::list<SubscriberInfo> Fib::lookupSubscription(const MediaNet::ShortName& name) const{
	auto result = std::list<SubscriberInfo>{};
	auto entries = fibStore.equal_range(name);
	std::for_each(entries.first, entries.second, [&result](auto& entry)
	{
			result.push_back(entry.second);
	});

	return result;

}