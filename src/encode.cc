#include <cassert>
#include <iostream>

#include "encode.hh"
#include "packet.hh"

using namespace MediaNet;

std::unique_ptr<Packet> &MediaNet::operator<<(std::unique_ptr<Packet> &p,
                                              const ShortName &msg) {

  int startSize = p->size();

  p << msg.fragmentID; // size = 1
  p << msg.mediaTime;  // size = 4
  p << msg.sourceID;   // size = 1
  p << msg.senderID;   // size = 4
  p << msg.resourceID; // size = 8

  int endSize = p->size();

  assert((endSize - startSize) == 18);

  p << PacketTag::shortName;

  return p;
}

bool MediaNet::operator>>(std::unique_ptr<Packet> &p, ShortName &msg) {
  if (nextTag(p) != PacketTag::shortName) {
    std::cerr << "Did not find expected PacketTag::shortName" << std::endl;
    return false;
  }

  PacketTag tag = PacketTag::none;
  bool ok = true;
  ok &= p >> tag;
  ok &= p >> msg.resourceID;
  ok &= p >> msg.senderID;
  ok &= p >> msg.sourceID;
  ok &= p >> msg.mediaTime;
  ok &= p >> msg.fragmentID;

  if (!ok) {
    std::cerr << "problem parsing shortName" << std::endl;
  }

  return ok;
}

std::unique_ptr<Packet> &MediaNet::operator<<(std::unique_ptr<Packet> &p,
                                              const ClientData &msg) {
  p << msg.clientSeqNum;
  p << PacketTag::clientData;

  return p;
}

bool MediaNet::operator>>(std::unique_ptr<Packet> &p, ClientData &msg) {
  if (nextTag(p) != PacketTag::clientData) {
    std::cerr << "Did not find expected PacketTag::ClientData" << std::endl;
    return false;
  }

  PacketTag tag = PacketTag::none;
  bool ok = true;
  ok &= p >> tag;
  ok &= p >> msg.clientSeqNum;

  if (!ok) {
    std::cerr << "problem parsing ClientData" << std::endl;
  }

  return ok;
}

/********* RelaySeqNum TAG ********/

std::unique_ptr<Packet> &MediaNet::operator<<(std::unique_ptr<Packet> &p,
                                              const RelayData &msg) {

  p << msg.relaySendTimeUs;
  p << msg.relaySeqNum;

  p << PacketTag::relayData;

  return p;
}

bool MediaNet::operator>>(std::unique_ptr<Packet> &p, RelayData &msg) {

  if (nextTag(p) != PacketTag::relayData) {
    std::cerr << "Did not find expected PacketTag::remoteSeqNum" << std::endl;
    return false;
  }

  PacketTag tag = PacketTag::none;
  bool ok = true;
  ok &= p >> tag;
  ok &= p >> msg.relaySeqNum;
  ok &= p >> msg.relaySendTimeUs;

  if (!ok) {
    std::cerr << "problem parsing RelayData" << std::endl;
  }

  return ok;
}

///
/// NetAck
///
std::unique_ptr<Packet> &MediaNet::operator<<(std::unique_ptr<Packet> &p,
                                              const NetAck &msg) {
  p << msg.ecnVec;
  p << msg.ackVec;
  p << msg.clientSeqNum;
  p << msg.recvTimeUs;
  p << PacketTag::ack;

  return p;
}

bool MediaNet::operator>>(std::unique_ptr<Packet> &p, NetAck &msg) {
  if (nextTag(p) != PacketTag::ack) {
    std::clog << "Did not find expected PacketTag::ack" << std::endl;
    return false;
  }

  PacketTag tag = PacketTag::none;
  bool ok = true;
  ok &= p >> tag;
  ok &= p >> msg.recvTimeUs;
  ok &= p >> msg.clientSeqNum;
  ok &= p >> msg.ackVec;
  ok &= p >> msg.ecnVec;

  if (!ok) {
    std::cerr << "problem parsing NetAck" << std::endl;
  }

  return ok;
}

///
/// NetNack
///
std::unique_ptr<Packet> &MediaNet::operator<<(std::unique_ptr<Packet> &p,
                                              const NetNack &msg) {
  p << msg.relaySeqNum;
  p << PacketTag::nack;

  return p;
}

bool MediaNet::operator>>(std::unique_ptr<Packet> &p, NetNack &msg) {
  if (nextTag(p) != PacketTag::nack) {
    std::clog << "Did not find expected PacketTag::nack" << std::endl;
    return false;
  }

  PacketTag tag = PacketTag::none;
  bool ok = true;
  ok &= p >> tag;
  ok &= p >> msg.relaySeqNum;

  if (!ok) {
    std::cerr << "problem parsing NetNack" << std::endl;
  }

  return ok;
}

///
/// Subscribe
///
std::unique_ptr<Packet> &MediaNet::operator<<(std::unique_ptr<Packet> &p,
                                              const Subscribe &msg) {
  p << msg.name;
  p << PacketTag::subscribe;

  return p;
}

bool MediaNet::operator>>(std::unique_ptr<Packet> &p, Subscribe &msg) {
  if (nextTag(p) != PacketTag::subscribe) {
    std::clog << "Did not find expected PacketTag::subscribe" << std::endl;
    return false;
  }

  PacketTag tag = PacketTag::none;
  bool ok = true;
  ok &= p >> tag;
  ok &= p >> msg.name;

  if (!ok) {
    std::cerr << "problem parsing Subscribe" << std::endl;
  }

  return ok;
}

/*************** TAG types *************************/

PacketTag MediaNet::nextTag(std::unique_ptr<Packet> &p) {
  if (p->fullSize() <= 0) {
    return PacketTag::none;
  }
  uint8_t truncTag = p->back(); // TODO - support var int size tags
  return nextTag(truncTag);
}

PacketTag MediaNet::nextTag(uint16_t truncTag) {
  PacketTag tag = PacketTag::badTag;
  switch (truncTag) {
  case packetTagTrunc(PacketTag::none):
    tag = PacketTag::none;
    break;
  case packetTagTrunc(PacketTag::sync):
    tag = PacketTag::sync;
    break;
  case packetTagTrunc(PacketTag::syncAck):
    tag = PacketTag::syncAck;
    break;

  case packetTagTrunc(PacketTag::reset):
    tag = PacketTag::reset;
    break;
  case packetTagTrunc(PacketTag::resetRetry):
    tag = PacketTag::resetRetry;
    break;
  case packetTagTrunc(PacketTag::resetRedirect):
    tag = PacketTag::resetRedirect;
    break;

  case packetTagTrunc(PacketTag::subscribe):
    tag = PacketTag::subscribe;
    break;
  case packetTagTrunc(PacketTag::clientData):
    tag = PacketTag::clientData;
    break;
  case packetTagTrunc(PacketTag::nack):
    tag = PacketTag::nack;
    break;
  case packetTagTrunc(PacketTag::rate):
    tag = PacketTag::rate;
    break;
  case packetTagTrunc(PacketTag::ack):
    tag = PacketTag::ack;
    break;
  case packetTagTrunc(PacketTag::relayData):
    tag = PacketTag::relayData;
    break;

  case packetTagTrunc(PacketTag::shortName):
    tag = PacketTag::shortName;
    break;
  case packetTagTrunc(PacketTag::dataBlock):
    tag = PacketTag::dataBlock;
    break;
  case packetTagTrunc(PacketTag::encDataBlock):
    tag = PacketTag::encDataBlock;
    break;


  case packetTagTrunc(PacketTag::subData):
    tag = PacketTag::subData;
    break;
  case packetTagTrunc(PacketTag::pubData):
    tag = PacketTag::pubData;
    break;


  case packetTagTrunc(PacketTag::headerData):
    tag = PacketTag::headerData;
    break;
  case packetTagTrunc(PacketTag::headerSyn):
    tag = PacketTag::headerSyn;
    break;
  case packetTagTrunc(PacketTag::headerSynAck):
    tag = PacketTag::headerSynAck;
    break;
  case packetTagTrunc(PacketTag::headerRst):
    tag = PacketTag::headerRst;
    break;
  case packetTagTrunc(PacketTag::headerDataCrazy):
    tag = PacketTag::headerDataCrazy;
    break;
  case packetTagTrunc(PacketTag::headerRstCrazy):
    tag = PacketTag::headerRstCrazy;
    break;
  case packetTagTrunc(PacketTag::headerSynCrazy):
    tag = PacketTag::headerSynCrazy;
    break;
  case packetTagTrunc(PacketTag::headerSynAckCrazy):
    tag = PacketTag::headerSynAckCrazy;
    break;

  case packetTagTrunc(PacketTag::badTag):
    tag = PacketTag::badTag;
    break;
  default:
    break;
  }

  return tag;
}

std::unique_ptr<Packet> &MediaNet::operator<<(std::unique_ptr<Packet> &p,
                                              PacketTag tag) {
  uint16_t t = MediaNet::packetTagTrunc(tag);
  assert(t < 127); // TODO var len encode
  p->push_back((uint8_t)t);
  return p;
}

bool MediaNet::operator>>(std::unique_ptr<Packet> &p, PacketTag &tag) {
  if (p->fullSize() == 0) {
    tag = PacketTag::none;
    return false;
  }

  tag = MediaNet::nextTag(p);

  p->pop_back();
  return true;
}

/******************    atomic types *************************/

std::unique_ptr<Packet> &MediaNet::operator<<(std::unique_ptr<Packet> &p,
                                              uint64_t val) {
  // TODO - std::copy version for little endian machines optimization

  // buffer on wire is little endian (that is *not* network byte order)
  p->push_back(uint8_t((val >> 0) & 0xFF));
  p->push_back(uint8_t((val >> 8) & 0xFF));
  p->push_back(uint8_t((val >> 16) & 0xFF));
  p->push_back(uint8_t((val >> 24) & 0xFF));
  p->push_back(uint8_t((val >> 32) & 0xFF));
  p->push_back(uint8_t((val >> 40) & 0xFF));
  p->push_back(uint8_t((val >> 48) & 0xFF));
  p->push_back(uint8_t((val >> 56) & 0xFF));

  return p;
}

std::unique_ptr<Packet> &MediaNet::operator<<(std::unique_ptr<Packet> &p,
                                              uint32_t val) {
  // buffer on wire is little endian (that is *not* network byte order)
  p->push_back(uint8_t((val >> 0) & 0xFF));
  p->push_back(uint8_t((val >> 8) & 0xFF));
  p->push_back(uint8_t((val >> 16) & 0xFF));
  p->push_back(uint8_t((val >> 24) & 0xFF));

  return p;
}

std::unique_ptr<Packet> &MediaNet::operator<<(std::unique_ptr<Packet> &p,
                                              uint16_t val) {
  // buffer on wire is little endian (that is *not* network byte order)
  p->push_back(uint8_t((val >> 0) & 0xFF));
  p->push_back(uint8_t((val >> 8) & 0xFF));

  return p;
}

std::unique_ptr<Packet> &MediaNet::operator<<(std::unique_ptr<Packet> &p,
                                              uint8_t val) {
  p->push_back(val);

  return p;
}

std::unique_ptr<Packet> &MediaNet::operator<<(std::unique_ptr<Packet> &p,
                                              const std::string &val) {
  // 1 byte to store the length of string
  assert(val.size() <= 255);
  auto valVec = std::vector<uint8_t>(val.begin(), val.end());
  p->push_back(valVec);
  p->push_back(valVec.size());
  return p;
}

std::unique_ptr<Packet> &MediaNet::operator<<(std::unique_ptr<Packet> &p,
                                              const std::vector<uint8_t> &val) {
  p->push_back(val);
  p->push_back(val.size());
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
  if (p->fullSize() == 0) {
    return false;
  }

  val = p->back();
  p->pop_back();
  return true;
}

bool MediaNet::operator>>(std::unique_ptr<Packet> &p, std::string &val) {
  if (p->fullSize() == 0) {
    return false;
  }
  uint8_t vecSize = 0;
  p >> vecSize;
  if (vecSize == 0) {
    return false;
  }
  auto vecData = p->back(vecSize);
  val.resize(vecSize);
  val.assign(vecData.begin(), vecData.end());
  return true;
}

bool MediaNet::operator>>(std::unique_ptr<Packet> &p,
                          std::vector<uint8_t> &val) {
  if (p->fullSize() == 0) {
    return false;
  }

  uint8_t vecSize = 0;
  p >> vecSize;
  if (vecSize == 0) {
    return false;
  }

  val.resize(vecSize);
  val = p->back(vecSize);
  return true;
}

///
/// Protocol messages
///

std::unique_ptr<Packet> &MediaNet::operator<<(std::unique_ptr<Packet> &p,
                                              const NetSyncReq &msg) {
  p << msg.supportedFeaturesVec;
  p << msg.clientTimeMs;
  p << msg.senderId;
  p << msg.origin;
  p << msg.cookie;

  p << PacketTag::sync;

  return p;
}

bool MediaNet::operator>>(std::unique_ptr<Packet> &p, NetSyncReq &msg) {
  if (nextTag(p) != PacketTag::sync) {
    std::cerr << "Did not find expected PacketTag::sync" << std::endl;
    return false;
  }

  PacketTag tag = PacketTag::none;
  bool ok = true;
  ok &= p >> tag;
  ok &= p >> msg.cookie;
  ok &= p >> msg.origin;
  ok &= p >> msg.senderId;
  ok &= p >> msg.clientTimeMs;
  ok &= p >> msg.supportedFeaturesVec;

  if (!ok) {
    std::cerr << "problem parsing sync" << std::endl;
  }

  return ok;
}

///
/// SyncAck
///
std::unique_ptr<Packet> &MediaNet::operator<<(std::unique_ptr<Packet> &p,
                                              const NetSyncAck &msg) {
  // TODO add other fields
  p << msg.useFeaturesVec;
  p << msg.serverTimeMs;
  p << PacketTag::syncAck;

  return p;
}

bool MediaNet::operator>>(std::unique_ptr<Packet> &p, NetSyncAck &msg) {
  if (nextTag(p) != PacketTag::syncAck) {
    std::cerr << "Did not find expected PacketTag::syncAck" << std::endl;
    return false;
  }

  PacketTag tag = PacketTag::none;
  bool ok = true;
  ok &= p >> tag;
  ok &= p >> msg.serverTimeMs;
  ok &= p >> msg.useFeaturesVec;
  if (!ok) {
    std::cerr << "problem parsing syncAck" << std::endl;
  }

  return ok;
}

///
/// NetReset and Types
///
std::unique_ptr<Packet> &MediaNet::operator<<(std::unique_ptr<Packet> &p,
                                              const NetRstRetry &msg) {

  p << msg.cookie;
  p << PacketTag::resetRetry;
  return p;
}

bool MediaNet::operator>>(std::unique_ptr<Packet> &p, NetRstRetry &msg) {
  if (nextTag(p) != PacketTag::resetRetry) {
    std::cerr << "Did not find expected PacketTag::RstRetry" << std::endl;
    return false;
  }

  PacketTag tag = PacketTag::none;
  bool ok = true;
  ok &= p >> tag;
  ok &= p >> msg.cookie;
  if (!ok) {
    std::cerr << "problem parsing RstRetry" << std::endl;
  }

  return ok;
}

std::unique_ptr<Packet> &MediaNet::operator<<(std::unique_ptr<Packet> &p,
                                              const NetRstRedirect &msg) {
  p << msg.port;
  p << msg.origin;
  p << msg.cookie;
  p << PacketTag::resetRedirect;
  return p;
}

bool MediaNet::operator>>(std::unique_ptr<Packet> &p, NetRstRedirect &msg) {
  if (nextTag(p) != PacketTag::resetRedirect) {
    std::cerr << "Did not find expected PacketTag::RstRedirect" << std::endl;
    return false;
  }

  PacketTag tag = PacketTag::none;
  bool ok = true;
  ok &= p >> tag;
  ok &= p >> msg.cookie;
  ok &= p >> msg.origin;
  ok &= p >> msg.port;

  if (!ok) {
    std::cerr << "problem parsing RstRedirect" << std::endl;
  }

  return ok;
}

///
/// NetRateReq
///
std::unique_ptr<Packet> &MediaNet::operator<<(std::unique_ptr<Packet> &p,
                                              const NetRateReq &msg) {
  p << msg.bitrateKbps;
  p << PacketTag::rate;

  return p;
}

bool MediaNet::operator>>(std::unique_ptr<Packet> &p, NetRateReq &msg) {
  if (nextTag(p) != PacketTag::rate) {
    // std::clog << "Did not find expected PacketTag::rate" <<
    // std::endl;
    return false;
  }

  PacketTag tag = PacketTag::rate;
  bool ok = true;

  ok &= p >> tag;
  ok &= p >> msg.bitrateKbps;

  if (!ok) {
    std::cerr << "problem parsing rate" << std::endl;
  }

  return ok;
}

///
/// EncryptedDataBlock
///

std::unique_ptr<Packet> &MediaNet::operator<<(std::unique_ptr<Packet> &p,
                                              const EncryptedDataBlock &data) {

  p << data.cipherDataLen;
  p << data.metaDataLen;
  p << data.authTagLen;

  p << PacketTag::encDataBlock;

  return p;
}

bool MediaNet::operator>>(std::unique_ptr<Packet> &p,
                          EncryptedDataBlock &data) {

  if (nextTag(p) != PacketTag::encDataBlock) {
    // std::clog << "Did not find expected PacketTag::encDataBlock" <<
    // std::endl;
    return false;
  }

  PacketTag tag = PacketTag::encDataBlock;
  bool ok = true;

  ok &= p >> tag;
  ok &= p >> data.authTagLen;
  ok &= p >> data.metaDataLen;
  ok &= p >> data.cipherDataLen;

  if (!ok) {
    std::cerr << "problem parsing EncryptedDataBlock" << std::endl;
  }

  return ok;
}


///
/// DataBlock
///


std::unique_ptr<Packet> &MediaNet::operator<<(std::unique_ptr<Packet> &p,
                                              const DataBlock &data) {

  p << data.dataLen;
  p << data.metaDataLen;
  p << PacketTag::dataBlock;

  return p;
}

bool MediaNet::operator>>(std::unique_ptr<Packet> &p,
                          DataBlock &data) {

  if (nextTag(p) != PacketTag::dataBlock) {
    // std::clog << "Did not find expected PacketTag::encDataBlock" <<
    // std::endl;
    return false;
  }

  PacketTag tag = PacketTag::dataBlock;
  bool ok = true;

  ok &= p >> tag;
  ok &= p >> data.metaDataLen;
  ok &= p >> data.dataLen;

  if (!ok) {
    std::cerr << "problem parsing EncryptedDataBlock" << std::endl;
  }

  return ok;
}



///
/// NamedDataChunk
///


std::unique_ptr<Packet> &MediaNet::operator<<(std::unique_ptr<Packet> &p,
                                              const NamedDataChunk &data) {

  p << data.lifetime;
  p << data.shortName;

  return p;
}

bool MediaNet::operator>>(std::unique_ptr<Packet> &p,
                          NamedDataChunk &data) {

  bool ok = true;

  ok &= p >> data.shortName;
  ok &= p >> data.lifetime;

  if (!ok) {
    std::cerr << "problem parsing NamedDataChunk" << std::endl;
  }

  return ok;
}

///
/// ClientData
///

/*
std::unique_ptr<Packet> &MediaNet::operator<<(std::unique_ptr<Packet> &p,
                                              const ClientData &data) {

  p << data.clientSeqNum;
  p << PacketTag::clientData;

  return p;
}

bool MediaNet::operator>>(std::unique_ptr<Packet> &p, ClientData &data) {
  if (nextTag(p) != PacketTag::clientData) {
    std::clog << "Did not find expected PacketTag::clientData" << std::endl;
    return false;
  }

  PacketTag tag = PacketTag::none;
  bool ok = true;

  ok &= p >> tag;
  ok &= p >> data.clientSeqNum;

  if (!ok) {
    std::cerr << "problem parsing ClientData" << std::endl;
  }

  return ok;
}
*/

///
/// RelayData
///

/*
std::unique_ptr<Packet> &MediaNet::operator<<(std::unique_ptr<Packet> &p,
                                              const RelayData &data) {

  p << data.relaySendTimeUs;
  p << data.relaySeqNum;
  p << PacketTag::relayData;

  return p;
}

bool MediaNet::operator>>(std::unique_ptr<Packet> &p, RelayData &data) {
  if (nextTag(p) != PacketTag::relayData) {
    std::clog << "Did not find expected PacketTag::relayData" << std::endl;
    return false;
  }

  PacketTag tag = PacketTag::none;
  bool ok = true;

  ok &= p >> tag;
  ok &= p >> data.relaySeqNum;
  ok &= p >> data.relaySendTimeUs;

  if (!ok) {
    std::cerr << "problem parsing relayData" << std::endl;
  }

  return ok;
}
*/


///
/// var-ints
///

std::unique_ptr<Packet> &MediaNet::operator<<(std::unique_ptr<Packet> &p,
                                              uintVar_t v) {
  uint64_t val = fromVarInt(v);

  assert(val < ((uint64_t)1 << 61));

  if (val <= ((uint64_t)1 << 7)) {
    p->push_back(uint8_t(((val >> 0) & 0x7F)) | 0x00);
    return p;
  }

  if (val <= ((uint64_t)1 << 14)) {
    p->push_back(uint8_t((val >> 0) & 0xFF));
    p->push_back(uint8_t(((val >> 8) & 0x3F) | 0x80));
    return p;
  }

  if (val <= ((uint64_t)1 << 29)) {
    p->push_back(uint8_t((val >> 0) & 0xFF));
    p->push_back(uint8_t((val >> 8) & 0xFF));
    p->push_back(uint8_t((val >> 16) & 0xFF));
    p->push_back(uint8_t(((val >> 24) & 0x1F) | 0x80 | 0x40));
    return p;
  }

  p->push_back(uint8_t((val >> 0) & 0xFF));
  p->push_back(uint8_t((val >> 8) & 0xFF));
  p->push_back(uint8_t((val >> 16) & 0xFF));
  p->push_back(uint8_t((val >> 24) & 0xFF));
  p->push_back(uint8_t((val >> 32) & 0xFF));
  p->push_back(uint8_t((val >> 40) & 0xFF));
  p->push_back(uint8_t((val >> 48) & 0xFF));
  p->push_back(uint8_t(((val >> 56) & 0x0F) | 0x80 | 0x40 | 0x20));

  return p;
}

bool MediaNet::operator>>(std::unique_ptr<Packet> &p, uintVar_t &v) {
  uint8_t byte[8] = {0, 0, 0, 0, 0, 0, 0, 0};
  bool ok = true;

  if (p->fullSize() == 0) {
    return false;
  }
  uint8_t first = p->back();

  if ((first & (0x80)) == 0) {
    ok &= p >> byte[0];
    uint8_t val = ((byte[0] & 0x7F) << 0);
    v = toVarInt(val);
    return ok;
  }

  if ((first & (0x80 | 0x40)) == 0x80) {
    ok &= p >> byte[1];
    ok &= p >> byte[0];
    uint16_t val = (((uint16_t)byte[1] & 0x3F) << 8) + ((uint16_t)byte[0] << 0);
    v = toVarInt(val);
    return ok;
  }

  if ((first & (0x80 | 0x40 | 0x20)) == (0x80 | 0x40)) {
    ok &= p >> byte[3];
    ok &= p >> byte[2];
    ok &= p >> byte[1];
    ok &= p >> byte[0];
    uint32_t val = ((uint32_t)(byte[3] & 0x1F) << 24) +
                   ((uint32_t)byte[2] << 16) + ((uint32_t)byte[1] << 8) +
                   ((uint32_t)byte[0] << 0);
    v = toVarInt(val);
    return ok;
  }

  ok &= p >> byte[7];
  ok &= p >> byte[6];
  ok &= p >> byte[5];
  ok &= p >> byte[4];
  ok &= p >> byte[3];
  ok &= p >> byte[2];
  ok &= p >> byte[1];
  ok &= p >> byte[0];
  uint64_t val = ((uint64_t)(byte[3] & 0x0F) << 56) +
                 ((uint64_t)(byte[2]) << 48) + ((uint64_t)(byte[1]) << 40) +
                 ((uint64_t)(byte[0]) << 32) + ((uint64_t)(byte[2]) << 24) +
                 ((uint64_t)(byte[2]) << 16) + ((uint64_t)(byte[1]) << 8) +
                 ((uint64_t)(byte[0]) << 0);
  v = toVarInt(val);
  return ok;
}

uintVar_t MediaNet::toVarInt(uint64_t v) {
  assert(v < ((uint64_t)0x1 << 61));
  return static_cast<uintVar_t>(v);
}

uint64_t MediaNet::fromVarInt(uintVar_t v) { return static_cast<uint64_t>(v); }

std::ostream &MediaNet::operator<<(std::ostream &stream, Packet &packet) {
  // TODO need to update this and perhaps change to "toString"

  int ptr = (int)packet.fullSize() - 1;
  while (ptr >= 0) {
    const uint8_t *data = &(packet.fullData());
    MediaNet::PacketTag tag = nextTag(data[ptr--]);
    uint16_t len = ((uint16_t)tag) & 0xFF;
    if (len == 255) {
      if (ptr >= 2) {
        uint16_t lenHigh = data[ptr--];
        uint16_t lenLow = data[ptr--];
        len = (lenHigh << 8) + lenLow;
      }
    }

    ptr -= len;

    switch (tag) {
    case PacketTag::headerData:
    case PacketTag::headerDataCrazy:
      stream << " magicData";
      break;
    case PacketTag::headerSyn:
    case PacketTag::headerSynCrazy:
      stream << " magicSync";
      break;
    case PacketTag::headerSynAck:
    case PacketTag::headerSynAckCrazy:
      stream << " magicSyncAck";
      break;
    case PacketTag::headerRst:
    case PacketTag::headerRstCrazy:
      stream << " magicReset";
      break;
    case PacketTag::sync:
      stream << " sync";
      break;
    case PacketTag::syncAck:
      stream << " syncAck";
      break;
    case PacketTag::resetRetry:
      stream << " resetRetry";
      break;
    case PacketTag::resetRedirect:
      stream << " resetRedirect";
      break;
    case PacketTag::shortName:
      stream << " shortName";
      break;
    case PacketTag::pubData:
      stream << " pubData(" << len << ")";
      break;
      //    case PacketTag::pubDataFrag:
      //      stream << " pubDataFrag(" << len << ")";
      //      break;
    case PacketTag::clientData:
      stream << " clientData";
      break;
    case PacketTag::relayData:
      stream << " relayData";
      break;
    default:
      stream << " tag:" << (((uint16_t)tag) >> 8) << "(" << len << ")";
    }
    if (((uint16_t)tag) == 0) {
      ptr = -1;
    }
  }
  return stream;
}

size_t Packet::size() const { return buffer.size() - headerSize; }

[[maybe_unused]] uint8_t Packet::getPriority() const { return priority; }

void Packet::setPriority(uint8_t priorityVal ) { Packet::priority = priorityVal; }

bool Packet::getFEC() const { return useFEC; }
