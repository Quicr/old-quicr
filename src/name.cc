
#include "name.hh"

using namespace MediaNet;

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
