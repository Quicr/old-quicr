#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include "packetTag.hh"
#include "name.hh"

namespace MediaNet {

class Packet;

std::unique_ptr<Packet> &operator<<(std::unique_ptr<Packet> &p, uint64_t val);
std::unique_ptr<Packet> &operator<<(std::unique_ptr<Packet> &p, uint32_t val);
std::unique_ptr<Packet> &operator<<(std::unique_ptr<Packet> &p, uint16_t val);
std::unique_ptr<Packet> &operator<<(std::unique_ptr<Packet> &p, uint8_t val);
std::unique_ptr<Packet> &operator<<(std::unique_ptr<Packet> &p,
                                    const std::string &val);
std::unique_ptr<Packet> &operator<<(std::unique_ptr<Packet> &p,
                                    const std::vector<uint8_t> &val);

bool operator>>(std::unique_ptr<Packet> &p, uint64_t &val);
bool operator>>(std::unique_ptr<Packet> &p, uint32_t &val);
bool operator>>(std::unique_ptr<Packet> &p, uint16_t &val);
bool operator>>(std::unique_ptr<Packet> &p, uint8_t &val);
bool operator>>(std::unique_ptr<Packet> &p, std::string &val);
bool operator>>(std::unique_ptr<Packet> &p, std::vector<uint8_t> &val);

enum class uintVar_t : uint64_t {};
std::unique_ptr<Packet> &operator<<(std::unique_ptr<Packet> &p, uintVar_t val);
bool operator>>(std::unique_ptr<Packet> &p, uintVar_t &val);
uintVar_t toVarInt(uint64_t);
uint64_t fromVarInt(uintVar_t);


PacketTag nextTag(std::unique_ptr<Packet> &p);
PacketTag nextTag(uint16_t truncTag);

bool operator>>(std::unique_ptr<Packet> &p, PacketTag &tag);
std::unique_ptr<Packet> &operator<<(std::unique_ptr<Packet> &p, PacketTag tag);

/* ShortName */
std::unique_ptr<Packet> &operator<<(std::unique_ptr<Packet> &p,
                                    const ShortName &msg);
bool operator>>(std::unique_ptr<Packet> &p, ShortName &msg);

/* Common header for all the messages */
struct Header {
	uint64_t pathToken;
	PacketTag headerMagic;
};
std::unique_ptr<Packet> &operator<<(std::unique_ptr<Packet> &p,
																		const Header &msg);
bool operator>>(std::unique_ptr<Packet> &p, Header &msg);

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

/* SYN ACK */
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

struct NetRstRetry : NetReset {};

struct NetRstRedirect : NetReset {
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

/* NetAck */
struct NetAck {
  uint32_t ackVec;
  uint32_t ecnVec;
  uint32_t clientSeqNum;
  uint32_t netRecvTimeUs;
};

std::unique_ptr<Packet> &operator<<(std::unique_ptr<Packet> &p,
                                    const NetAck &msg);
bool operator>>(std::unique_ptr<Packet> &p, NetAck &msg);

/* NetNack */
struct NetNack {
  uint32_t relaySeqNum;
};

std::unique_ptr<Packet> &operator<<(std::unique_ptr<Packet> &p,
                                    const NetNack &msg);
bool operator>>(std::unique_ptr<Packet> &p, NetNack &msg);

/* SubscribeRequest */
struct Subscribe {
  ShortName name;
};

std::unique_ptr<Packet> &operator<<(std::unique_ptr<Packet> &p,
                                    const Subscribe &msg);
bool operator>>(std::unique_ptr<Packet> &p, Subscribe &msg);

///
/// ClientData
///
struct ClientData {
  uint32_t clientSeqNum;
};

std::unique_ptr<Packet> &operator<<(std::unique_ptr<Packet> &p,
                                    const ClientData &msg);
bool operator>>(std::unique_ptr<Packet> &p, ClientData &msg);

///
/// RelayData
///
struct RelayData {
  uint32_t relaySeqNum;
  uint32_t remoteSendTimeUs;
};
std::unique_ptr<Packet> &operator<<(std::unique_ptr<Packet> &p,
                                    const RelayData &msg);
bool operator>>(std::unique_ptr<Packet> &p, RelayData &msg);

///
/// EncDataBlock
///
struct EncryptedDataBlock {
  uint16_t authTagLen;
  std::vector<uint8_t> cipherText; // enc + auth
};
std::unique_ptr<Packet> &operator<<(std::unique_ptr<Packet> &p,
                                    const EncryptedDataBlock &data);
bool operator>>(std::unique_ptr<Packet> &p, EncryptedDataBlock &msg);

///
/// PubSubData and friends
///
struct PublishedData {
  ShortName name;
  uintVar_t lifetime;
  EncryptedDataBlock encryptedDataBlock;
};

// sent by client in publish message
struct PubData : public PublishedData {};

std::unique_ptr<Packet> &operator<<(std::unique_ptr<Packet> &p,
                                    const PubData &msg);
bool operator>>(std::unique_ptr<Packet> &p, PubData &msg);

// sent by relay to all the subscribers
struct SubData : public PublishedData {};

std::unique_ptr<Packet> &operator<<(std::unique_ptr<Packet> &p,
                                    const SubData &msg);
bool operator>>(std::unique_ptr<Packet> &p, SubData &msg);

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


struct NetRedirect4RResp {
  // no sourceId as sent from server
  uint32_t clientSeqNum;
  uint64_t clientToken;
  uint32_t ipAddr;
  uint16_t port;
};

*/

std::ostream &operator<<(std::ostream &stream, Packet &packet);

} // namespace MediaNet
