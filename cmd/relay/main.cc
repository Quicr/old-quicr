#include "include/relay.hh"
#include "include/fib.hh"
#include "include/multimap_fib.hh"
#include <iostream>


int main() {
	auto relay = Relay{5004};
	Fib *fib = new MultimapFib();
	MediaNet::ShortName subscription;
	subscription.resourceID = 0x1234;

	// subscribers
	auto info1 = SubscriberInfo{1};
	auto info2 = SubscriberInfo{2};
	auto info3 = SubscriberInfo{3};

	fib->addSubscription(subscription, info1);
	fib->addSubscription(subscription, info2);

	subscription.resourceID = 0x1111;
	fib->addSubscription(subscription, info3);

	// publisher
	MediaNet::ShortName publish_name;
	publish_name.resourceID = 0x1234;
	publish_name.senderID = 0x777;

	// retrieve all the subscribers
	auto result = fib->lookupSubscription(publish_name);
	if(result.size() != 2) {
		throw std::runtime_error("subscriptions mismatch");
	}

	fib->removeSubscription(subscription, info2);
	std::clog <<"Num Subscriptions:" << result.size() << "\n";
	while(1) {
		relay.process();
	}
}