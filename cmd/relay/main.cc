#include "include/relay.hh"
#include "include/fib.hh"
#include <iostream>


int main() {
	auto relay = Relay{5004};
	Fib fib;
	MediaNet::ShortName name;
	name.resourceID = 0x1234;
	auto info1 = SubscriberInfo{1};
	auto info2 = SubscriberInfo{2};
	auto info3 = SubscriberInfo{3};

	fib.addSubscription(name, info1);
	fib.addSubscription(name, info1);
	fib.addSubscription(name, info2);

	auto result = fib.lookupSubscription(name);
	fib.removeSubscription(name, info2);
	while(1) {
		relay.process();
	}
}