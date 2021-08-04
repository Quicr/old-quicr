#pragma once

#include <list>
#include <map>

#include "fib.hh"

// FibStore driven by std::multimap
struct MultimapFib : public Fib
{
public:
  virtual void addSubscription(const MediaNet::ShortName& name,
                               SubscriberInfo subscriberInfo) override;
  virtual void removeSubscription(
    const MediaNet::ShortName& name,
    const SubscriberInfo& subscriberInfo) override;
  virtual std::list<SubscriberInfo> lookupSubscription(
    const MediaNet::ShortName& name) const override;

private:
  std::multimap<MediaNet::ShortName, SubscriberInfo> fibStore;
};
