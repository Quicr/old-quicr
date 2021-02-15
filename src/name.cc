
#include "name.hh"

using namespace MediaNet;

ShortName::ShortName(  ):
    resourceID(0), senderID(0),sourceID(0),mediaTime(0),fragmentID(0)
{}

ShortName::ShortName( uint64_t rid ):
    resourceID(rid), senderID(0),sourceID(0),mediaTime(0),fragmentID(0)
{}


ShortName::ShortName( uint64_t rid,  uint32_t sender):
    resourceID(rid), senderID(sender),sourceID(0),mediaTime(0),fragmentID(0)
{}

ShortName::ShortName( uint64_t rid,  uint32_t sender,  uint8_t source ):
  resourceID(rid), senderID(sender),sourceID(source),mediaTime(0),fragmentID(0)
{}


std::ostream &MediaNet::operator<<(std::ostream &stream,
                                   const ShortName &name) {

  stream << "qr:./r" << name.resourceID << "/c" << name.senderID << "/s"
         << (uint16_t)name.sourceID << "/" << name.mediaTime;

  if (name.fragmentID != 0) {
    if ((name.fragmentID) & 0x1) {
      stream << "*" << (uint16_t)name.fragmentID / 2;
    } else {
      stream << "+" << (uint16_t)name.fragmentID / 2;
    }
  }

  return stream;
}

// Note: no parsing of fragment and mediaTime is supported
// since they are calculated via code.
ShortName ShortName::fromString(const std::string &name_str){
	assert(!name_str.empty());

	ShortName name;
	const std::string proto = "qr://";
	auto it = std::search(name_str.begin(), name_str.end(), proto.begin(), proto.end());
	assert(it != name_str.end());

	//move to end for qr://
	std::advance(it, proto.length());

	std::string resource_str;
	std::string client_str;
	std::string source_str;

	do {
		auto slash = std::find(it, name_str.end(), '/');
		if(slash == name_str.end()) {
			break;
		}

		if(resource_str.empty()) {
			// parse resource id
			resource_str.reserve(distance(it, slash));
			resource_str.assign(it, slash);
			std::advance(it, resource_str.length());
			name.resourceID = stoi(resource_str, nullptr);
			it++;
			continue;
		}

		if(client_str.empty()) {
			// parse client/sender id
			client_str.reserve(distance(it, slash));
			client_str.assign(it, slash);
			std::advance(it, client_str.length());
			name.senderID = stoi(client_str, nullptr);
			it++;
			continue;
		}

		if(source_str.empty()) {
			// parse sourceId
			source_str.reserve(distance(it, slash));
			source_str.assign(it, slash);
			std::advance(it, source_str.length());
			name.sourceID = stoi(source_str, nullptr);
			it++;
			continue;
		}

	} while(it != name_str.end());


	return name;
}
