#pragma once

#include <cstdint>
#include <memory>

#include "name.hh"

namespace MediaNet {

class Packet;

std::unique_ptr<Packet> &operator<<(std::unique_ptr<Packet> &p, uint64_t val);
std::unique_ptr<Packet> &operator<<(std::unique_ptr<Packet> &p, uint32_t val);
std::unique_ptr<Packet> &operator<<(std::unique_ptr<Packet> &p, uint16_t val);
std::unique_ptr<Packet> &operator<<(std::unique_ptr<Packet> &p, uint8_t val);
std::unique_ptr<Packet>& operator<<(std::unique_ptr<Packet> &p, const std::string& val);

bool operator>>(std::unique_ptr<Packet> &p, uint64_t &val);
bool operator>>(std::unique_ptr<Packet> &p, uint32_t &val);
bool operator>>(std::unique_ptr<Packet> &p, uint16_t &val);
bool operator>>(std::unique_ptr<Packet> &p, uint8_t &val);
bool operator>>(std::unique_ptr<Packet> &p, std::string &val);

enum class uintVar_t : uint64_t {};
std::unique_ptr<Packet> &operator<<(std::unique_ptr<Packet> &p, uintVar_t val);
bool operator>>(std::unique_ptr<Packet> &p, uintVar_t &val);
uintVar_t toVarInt(uint64_t);
uint64_t fromVarInt(uintVar_t);

enum struct PacketTag : uint32_t;

constexpr unsigned int packetTagGen(unsigned int val, unsigned int len,
                                    bool mandToUnderstand) {
  (void)mandToUnderstand;
  return (val << 8) + len;
}
constexpr uint16_t packetTagTrunc(MediaNet::PacketTag tag) {
  auto t = (uint32_t)tag;
  t >>= 8;
  return (uint16_t)t;
}

enum struct PacketTag : uint32_t {
  /*
   * If you add a tag, remember to to add MediaNet::nextTag decoder
   * A length of 255 in packerTagGen means variable length data
   * */

  none = packetTagGen(0, 0, true), // must be smallest tag

  appData = packetTagGen(1, 255, true),
  appDataFrag = packetTagGen(9, 255, true),
  clientSeqNum = packetTagGen(2, 4, true), // make part of appData ???
  ack = packetTagGen(3, 255, true),
  sync = packetTagGen(4, 255, true),
  shortName = packetTagGen(5, 18, true),
  relaySeqNum = packetTagGen(7, 8, true),
  relayRateReq = packetTagGen(6, 4, true),
  subscribeReq = packetTagGen(8, 0, true),
	syncAck = packetTagGen(10, 255, true),
	rstRetry = packetTagGen(11, 255, true),
	rstRedirect = packetTagGen(12, 255, true),

  // TODO - Could add nextReservedCodePoints of various lengths and MTI

  // This block of headerMagic values selected to multiplex with STUN/DTLS/RTP
  headerMagicData = packetTagGen(16, 0, true),
  headerMagicSyn = packetTagGen(18, 0, true),
	headerMagicSynAck = packetTagGen(20, 0, true),
	headerMagicRst = packetTagGen(22, 0, true),
  headerMagicDataCrazy = packetTagGen(17, 0, true),
  headerMagicSynCrazy = packetTagGen(19, 0, true),
	headerMagicSynAckCrazy = packetTagGen(21, 0, true),
  headerMagicRstCrazy = packetTagGen(23, 0, true),

  extraMagicVer1 = packetTagGen(12538, 0, false),

  badTag = packetTagGen(16383, 0,
                        true), // must not have any tag values greater than this
};

PacketTag nextTag(std::unique_ptr<Packet> &p);
PacketTag nextTag(uint16_t truncTag);

bool operator>>(std::unique_ptr<Packet> &p, PacketTag &tag);
std::unique_ptr<Packet> &operator<<(std::unique_ptr<Packet> &p, PacketTag tag);

/* ShortName */

std::unique_ptr<Packet> &operator<<(std::unique_ptr<Packet> &p,
                                    const ShortName &msg);

bool operator>>(std::unique_ptr<Packet> &p, ShortName &msg);

/* SYNC Request */
struct NetSyncReq {
	uint32_t cookie;
	std::string origin;
  uint32_t senderId;
  uint64_t clientTimeMs;
  uint64_t supportedFeaturesVec;
};
std::unique_ptr<Packet> &operator<<(std::unique_ptr<Packet> &p,
                                    const NetSyncReq &msg);
bool operator>>(std::unique_ptr<Packet> &p, NetSyncReq &msg);

/* NetSyncAck */
struct NetSyncAck {
	uint64_t serverTimeMs;
	uint64_t useFeaturesVec;
};

std::unique_ptr<Packet> &operator<<(std::unique_ptr<Packet> &p,
																		const NetSyncAck &msg);
bool operator>>(std::unique_ptr<Packet> &p, NetSyncAck &msg);


/* NetReset*/
struct NetReset {
	uint64_t cookie;
};

struct NetRstRetry: NetReset {};

struct NetRstRedirect: NetReset {
	std::string origin;
	uint16_t port;
};

std::unique_ptr<Packet> &operator<<(std::unique_ptr<Packet> &p,
																		const NetRstRetry &msg);
bool operator>>(std::unique_ptr<Packet> &p, NetRstRetry &msg);
std::unique_ptr<Packet> &operator<<(std::unique_ptr<Packet> &p,
																		const NetRstRedirect &msg);
bool operator>>(std::unique_ptr<Packet> &p, NetRstRedirect &msg);


/* Rate Request */

struct NetRateReq {
  uintVar_t bitrateKbps; // in kilo bits pers second
};
std::unique_ptr<Packet> &operator<<(std::unique_ptr<Packet> &p,
                                    const NetRateReq &msg);
bool operator>>(std::unique_ptr<Packet> &p, NetRateReq &msg);

/*
 * struct NetNonceReq {
  uint32_t sourceId;
  uint64_t timeNowMs;
};
struct NetNonceResp {
  uint64_t clientToken;
  bool timeOK;
  bool sourceIdOK;
};
struct NetAuthReq {
  uint32_t sourceId;
  uint64_t timeNowMs;
  uint64_t clientToken;
  uint64_t authHash;
};
struct NetAuthResp {
  bool authOK;
};
*/

/*
struct NetRedirect4RResp {
  // no sourceId as sent from server
  uint32_t clientSeqNum;
  uint64_t clientToken;
  uint32_t ipAddr;
  uint16_t port;
};
*/

struct NetClientSeqNum {
  uint32_t clientSeqNum;
};
std::unique_ptr<Packet> &operator<<(std::unique_ptr<Packet> &p,
                                    const NetClientSeqNum &msg);
bool operator>>(std::unique_ptr<Packet> &p, NetClientSeqNum &msg);

struct NetRelaySeqNum {
  uint32_t relaySeqNum;
  uint32_t remoteSendTimeUs;
};
std::unique_ptr<Packet> &operator<<(std::unique_ptr<Packet> &p,
                                    const NetRelaySeqNum &msg);
bool operator>>(std::unique_ptr<Packet> &p, NetRelaySeqNum &msg);

struct NetAck {
  // todo - add ack and ECN vectors
  uint32_t netAckSeqNum;
  uint32_t netRecvTimeUs;
};

std::unique_ptr<Packet> &operator<<(std::unique_ptr<Packet> &p,
                                    const NetAck &msg);
bool operator>>(std::unique_ptr<Packet> &p, NetAck &msg);

/*
struct NetMsgSubReq {
  uint32_t subSourceId;
  uint8_t subMediaType; // encode 5 bits
  uint8_t subFlowID;    // encode 3 bits
  uint32_t zero;        // encode 12+12 bits of 0s to match header

  const uint16_t magic = 0xF10F;

  uint32_t sourceId;
  uint64_t timeNowMs;
  uint64_t clientToken;
  uint64_t authHash;
}; */

/*
struct NetMsgSubResp {
  uint32_t sourceId;
  uint8_t mediaType; // encode 5 bits
  uint8_t flowID;    // encode 3 bits
  uint32_t zero;     // encode 12+12 bits of 0s to match header

  const uint16_t magic = 0xF10F;

  bool authOK;      // encode 8 bit
  bool subscribeOK; // encode 8 bit
};
*/

/*
struct NetMsgClientStats {
  uint32_t sourceId;
  uint64_t timeNowMs;
  uint8_t statsVersion;

  uint32_t numAckRecv;
  uint32_t numNackRecv;
  uint32_t numBytesRecv;

  uint32_t numAckSent;
  uint32_t numNackSent;
  uint32_t numBytesSent;

  uint16_t rttEstMs;
  uint32_t upstreamBandwidthEstBps;
  uint32_t downstreamBandwidthEstBps;

  uint32_t reserved1;
  uint32_t reserved2;
};
*/

std::ostream &operator<<(std::ostream &stream, Packet &packet);

} // namespace MediaNet
