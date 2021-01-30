#include <cassert>
#include <iostream>
#include <tuple>

#if defined(__linux) || defined(__APPLE__)
#include <arpa/inet.h>
#include <netdb.h>
#endif
#if defined(__linux__)
#include <net/ethernet.h>
#include <netpacket/packet.h>
#elif defined(__APPLE__)
#include <arpa/inet.h>
#include <net/if_dl.h>
#elif defined(_WIN32)
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

#include "packet.hh"

using namespace MediaNet;

Packet::Packet() : headerSize(0), priority(1), reliable(false), useFEC(false) {
  src.addrLen = 0;
  dst.addrLen = 0;
  buffer.reserve(1480 /* MTU estimate */);
  name.fragmentID = 0;
  name.mediaTime = 0;
  name.sourceID = 0;
  name.senderID = 0;
  name.resourceID = 0;
}

void Packet::setReliable(bool r) { Packet::reliable = r; }

void Packet::enableFEC(bool doFec) { useFEC = doFec; }

const IpAddr &Packet::getSrc() const { return src; }

[[maybe_unused]] void Packet::setSrc(const IpAddr &srcAddr) { src = srcAddr; }

[[maybe_unused]] const IpAddr &Packet::getDst() const { return dst; }

void Packet::setDst(const IpAddr &dstAddr) { dst = dstAddr; }

void Packet::copy(const Packet &p) {
  name = p.name;
  buffer = p.buffer;
  headerSize = p.headerSize;
  priority = p.priority;
  reliable = p.reliable;
  useFEC = p.useFEC;

  src = p.src;
  dst = p.dst;
}

std::unique_ptr<Packet> Packet::clone() const {
  std::unique_ptr<Packet> p = std::make_unique<Packet>();

  p->copy(*this);

  return p;
}

bool Packet::isReliable() const { return reliable; }

bool IpAddr::operator<(const IpAddr &rhs) const {
  assert(this->addr.sin_family == AF_INET);
  assert(rhs.addr.sin_family == AF_INET);
  if (this->addrLen != rhs.addrLen) {
    return this->addrLen < rhs.addrLen;
  }
  if (this->addr.sin_family != rhs.addr.sin_family) {
    return this->addr.sin_family < rhs.addr.sin_family;
  }
  if (this->addr.sin_addr.s_addr != rhs.addr.sin_addr.s_addr) {
    return this->addr.sin_addr.s_addr < rhs.addr.sin_addr.s_addr;
  }
  if (this->addr.sin_port != rhs.addr.sin_port) {
    return this->addr.sin_port < rhs.addr.sin_port;
  }
  return false;
}

std::string IpAddr::toString(const IpAddr &ipAddr) {
  char str[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &(ipAddr.addr.sin_addr), str, sizeof(str));

  unsigned short port = ntohs(ipAddr.addr.sin_port);

  std::string ret = std::string(str) + ":" + std::to_string(port);

  return ret;
}

bool MediaNet::operator<(const ShortName &a, const ShortName &b) {
  return std::tie(a.mediaTime, a.resourceID, a.senderID, a.sourceID,
                  a.fragmentID) < std::tie(b.mediaTime, b.resourceID,
                                           b.senderID, b.sourceID,
                                           b.fragmentID);
}

void Packet::setFragID(const uint8_t fragmentID) {
  name.fragmentID = fragmentID;
  if (buffer.size() > 19) {
    assert(buffer.at(19) == packetTagTrunc(PacketTag::shortName));
    buffer.at(18) = fragmentID;
  }
}
