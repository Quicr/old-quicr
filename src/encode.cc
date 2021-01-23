#include <cassert>
#include <iostream>

#include "encode.hh"
#include "packet.hh"

using namespace MediaNet;

std::unique_ptr<Packet> &MediaNet::operator<<(std::unique_ptr<Packet> &p,
                                              const Packet::ShortName &msg) {

  p << msg.resourceID;
  p << msg.senderID;
  p << msg.sourceID;
  p << msg.mediaTime;
  p << msg.fragmentID;

  p << PacketTag::shortName;

  return p;
}

std::unique_ptr<Packet> &MediaNet::operator<<(std::unique_ptr<Packet> &p,
                                              const NetSeqNumTag &msg) {
  p << msg.netSeqNum;
  p << PacketTag::clientSeqNum;

  return p;
}

bool MediaNet::operator>>(std::unique_ptr<Packet> &p, NetSeqNumTag &msg) {
  if (nextTag(p) != PacketTag::clientSeqNum) {
    std::cerr << "Did not find expected PacketTag::clientSeqNum"
              << std::endl;
    return false;
  }

  PacketTag tag = PacketTag::none;
  bool ok = true;
  ok &= p >> tag;
  ok &= p >> msg.netSeqNum;

  if (!ok) {
    std::cerr << "problem parsing NetSeqNumTag" << std::endl;
  }

  return ok;
}

/********* RelaySeqNum TAG ********/

std::unique_ptr<Packet> &MediaNet::operator<<(std::unique_ptr<Packet> &p,
                                              const NetRelaySeqNum &msg) {

  p << msg.relaySeqNum;
  p << msg.remoteSendTimeMs;

  p << PacketTag::relaySeqNum;

  return p;
}

bool MediaNet::operator>>(std::unique_ptr<Packet> &p, NetRelaySeqNum &msg) {

  if (nextTag(p) != PacketTag::relaySeqNum) {
    std::cerr << "Did not find expected PacketTag::relaySeqNum" << std::endl;
    return false;
  }

  PacketTag tag = PacketTag::none;
  bool ok = true;
  ok &= p >> tag;
  ok &= p >> msg.remoteSendTimeMs;
  ok &= p >> msg.relaySeqNum;

  if (!ok) {
    std::cerr << "problem parsing NetRelaySeqNum" << std::endl;
  }

  return ok;
}

/********* ACK TAG ***************/

std::unique_ptr<Packet> &MediaNet::operator<<(std::unique_ptr<Packet> &p,
                                              const NetAckTag &msg) {
  p << msg.netAckSeqNum;
  p << msg.netRecvTimeUs;
  p << PacketTag::ack;

  return p;
}

bool MediaNet::operator>>(std::unique_ptr<Packet> &p, NetAckTag &msg) {
  if (nextTag(p) != PacketTag::ack) {
    // std::cerr << "Did not find expected PacketTag::ack" << std::endl;
    return false;
  }

  PacketTag tag = PacketTag::none;
  bool ok = true;
  ok &= p >> tag;
  ok &= p >> msg.netRecvTimeUs;
  ok &= p >> msg.netAckSeqNum;

  if (!ok) {
    std::cerr << "problem parsing NetAckTag" << std::endl;
  }

  return ok;
}

/*************** TAG types *************************/

PacketTag MediaNet::nextTag(std::unique_ptr<Packet> &p) {
  if (p->buffer.empty()) {
    return PacketTag::none;
  }
  uint8_t trucTag = p->buffer.back();
  PacketTag tag = PacketTag::badTag;
  switch (trucTag) {
  case packetTagTruc(PacketTag::none):
    tag = PacketTag::none;
    break;

  case packetTagTruc(PacketTag::appData):
    tag = PacketTag::appData;
    break;
  case packetTagTruc(PacketTag::clientSeqNum):
    tag = PacketTag::clientSeqNum;
    break;
  case packetTagTruc(PacketTag::ack):
    tag = PacketTag::ack;
    break;

  case packetTagTruc(PacketTag::sync):
    tag = PacketTag::sync;
    break;
  case packetTagTruc(PacketTag::shortName):
    tag = PacketTag::shortName;
    break;
  case packetTagTruc(PacketTag::relaySeqNum):
    tag = PacketTag::relaySeqNum;
    break;
  case packetTagTruc(PacketTag::relayRateReq):
    tag = PacketTag::relayRateReq;
    break;

  case packetTagTruc(PacketTag::headerMagicData):
    tag = PacketTag::headerMagicData;
    break;
  case packetTagTruc(PacketTag::headerMagicSyn):
    tag = PacketTag::headerMagicSyn;
    break;
  case packetTagTruc(PacketTag::headerMagicRst):
    tag = PacketTag::headerMagicRst;
    break;
  case packetTagTruc(PacketTag::headerMagicDataCrazy):
    tag = PacketTag::headerMagicDataCrazy;
    break;
  case packetTagTruc(PacketTag::headerMagicRstCrazy):
    tag = PacketTag::headerMagicRstCrazy;
    break;
  case packetTagTruc(PacketTag::headerMagicSynCrazy):
    tag = PacketTag::headerMagicSynCrazy;
    break;

  case packetTagTruc(PacketTag::extraMagicVer1):
    tag = PacketTag::extraMagicVer1;
    break;

  case packetTagTruc(PacketTag::badTag):
    tag = PacketTag::badTag;
    break;
  default:
    assert(0); // TODO remove
    tag = PacketTag::badTag;
  }

  return tag;
}

std::unique_ptr<Packet> &MediaNet::operator<<(std::unique_ptr<Packet> &p,
                                              PacketTag tag) {
  uint16_t t = MediaNet::packetTagTruc(tag);
  assert(t < 127); // TODO var len encode
  p->buffer.push_back((uint8_t)t);
  return p;
}

bool MediaNet::operator>>(std::unique_ptr<Packet> &p, PacketTag &tag) {
  if (p->buffer.empty()) {
    tag = PacketTag::none;
    return false;
  }

  tag = MediaNet::nextTag(p);

  p->buffer.pop_back();
  return true;
}

/******************    atomic types *************************/

std::unique_ptr<Packet> &MediaNet::operator<<(std::unique_ptr<Packet> &p,
                                              uint64_t val) {
  // TODO - memcpy version for little endian machines optimization

  // buffer on wire is little endian (that is *not* network byte order)
  p->buffer.push_back(uint8_t((val >> 0) & 0xFF));
  p->buffer.push_back(uint8_t((val >> 8) & 0xFF));
  p->buffer.push_back(uint8_t((val >> 16) & 0xFF));
  p->buffer.push_back(uint8_t((val >> 24) & 0xFF));
  p->buffer.push_back(uint8_t((val >> 32) & 0xFF));
  p->buffer.push_back(uint8_t((val >> 40) & 0xFF));
  p->buffer.push_back(uint8_t((val >> 48) & 0xFF));
  p->buffer.push_back(uint8_t((val >> 56) & 0xFF));

  return p;
}

std::unique_ptr<Packet> &MediaNet::operator<<(std::unique_ptr<Packet> &p,
                                              uint32_t val) {
  // buffer on wire is little endian (that is *not* network byte order)
  p->buffer.push_back(uint8_t((val >> 0) & 0xFF));
  p->buffer.push_back(uint8_t((val >> 8) & 0xFF));
  p->buffer.push_back(uint8_t((val >> 16) & 0xFF));
  p->buffer.push_back(uint8_t((val >> 24) & 0xFF));

  return p;
}

std::unique_ptr<Packet> &MediaNet::operator<<(std::unique_ptr<Packet> &p,
                                              uint16_t val) {
  // buffer on wire is little endian (that is *not* network byte order)
  p->buffer.push_back(uint8_t((val >> 0) & 0xFF));
  p->buffer.push_back(uint8_t((val >> 8) & 0xFF));

  return p;
}

std::unique_ptr<Packet> &MediaNet::operator<<(std::unique_ptr<Packet> &p,
                                              uint8_t val) {
  p->buffer.push_back(val);

  return p;
}

bool MediaNet::operator>>(std::unique_ptr<Packet> &p, uint64_t &val) {
  uint8_t byte[8] = {0, 0, 0, 0, 0, 0, 0, 0};

  bool ok = true;
  ok &= p >> byte[7];
  ok &= p >> byte[6];
  ok &= p >> byte[5];
  ok &= p >> byte[4];
  ok &= p >> byte[3];
  ok &= p >> byte[2];
  ok &= p >> byte[1];
  ok &= p >> byte[0];

  val = (uint64_t(byte[0]) << 0) + (uint64_t(byte[1]) << 8) +
        (uint64_t(byte[2]) << 16) + (uint64_t(byte[3]) << 24) +
        (uint64_t(byte[4]) << 32) + (uint64_t(byte[5]) << 40) +
        (uint64_t(byte[6]) << 48) + (uint64_t(byte[7]) << 56);

  return ok;
}

bool MediaNet::operator>>(std::unique_ptr<Packet> &p, uint32_t &val) {
  uint8_t byte[4] = {0, 0, 0, 0};

  bool ok = true;
  ok &= p >> byte[3];
  ok &= p >> byte[2];
  ok &= p >> byte[1];
  ok &= p >> byte[0];

  val = (byte[3] << 24) + (byte[2] << 16) + (byte[1] << 8) + (byte[0] << 0);

  return ok;
}

bool MediaNet::operator>>(std::unique_ptr<Packet> &p, uint16_t &val) {
  uint8_t byte[2] = {0, 0};

  bool ok = true;
  ok &= p >> byte[1];
  ok &= p >> byte[0];

  val = (byte[1] << 8) + (byte[0] << 0);

  return ok;
}

bool MediaNet::operator>>(std::unique_ptr<Packet> &p, uint8_t &val) {
  if (p->buffer.empty()) {
    return false;
  }
  val = p->buffer.back();
  p->buffer.pop_back();
  return true;
}

std::unique_ptr<Packet> &MediaNet::operator<<(std::unique_ptr<Packet> &p,
                                              const NetSynReq &msg) {
  p << msg.senderId;
  p << msg.clientTimeMs;
  p << msg.versionVec;

  p << PacketTag::sync;

  return p;
}

std::unique_ptr<Packet> &MediaNet::operator<<(std::unique_ptr<Packet> &p,
                                              const NetRateReq &msg) {
  p << msg.bitrateKbps;
  p << PacketTag::relayRateReq;

  return p;
}

bool MediaNet::operator>>(std::unique_ptr<Packet> &p, NetRateReq &msg) {
  if (nextTag(p) != PacketTag::relayRateReq) {
    // std::cerr << "Did not find expected PacketTag::relayRateReq" <<
    // std::endl;
    return false;
  }

  PacketTag tag = PacketTag::relayRateReq;
  bool ok = true;

  ok &= p >> tag;
  ok &= p >> msg.bitrateKbps;

  if (!ok) {
    std::cerr << "problem parsing relayRateReq" << std::endl;
  }

  return ok;
}