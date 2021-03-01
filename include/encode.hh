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
  uint32_t cookie;
};

struct NetRstRetry {
  uint32_t cookie;
};

struct NetRstRedirect : NetReset {
  uint32_t cookie;
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
  uint32_t clientSeqNum;
  uint32_t recvTimeUs;
  uint32_t ackVec;
  uint32_t ecnVec;
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
  uint32_t relaySendTimeUs;
};
std::unique_ptr<Packet> &operator<<(std::unique_ptr<Packet> &p,
                                    const RelayData &msg);
bool operator>>(std::unique_ptr<Packet> &p, RelayData &msg);

///
/// EncDataBlock
///
struct EncryptedDataBlock {
  uint8_t authTagLen;
  // varInt cipherTextAndTagLen;
  std::vector<uint8_t> cipherText; // enc + auth
};
std::unique_ptr<Packet> &operator<<(std::unique_ptr<Packet> &p,
                                    const EncryptedDataBlock &data);
bool operator>>(std::unique_ptr<Packet> &p, EncryptedDataBlock &msg);

///
/// DataBlock
///
struct DataBlock {
  uintVar_t metaDataLen;
  uintVar_t dataLen;
};
std::unique_ptr<Packet> &operator<<(std::unique_ptr<Packet> &p,
                                    const DataBlock &data);
bool operator>>(std::unique_ptr<Packet> &p, DataBlock &msg);


///
/// NamedDataChunk and friends
///
struct NamedDataChunk {
  ShortName name;
  uintVar_t lifetime;
  EncryptedDataBlock encryptedDataBlock;
};
std::unique_ptr<Packet> &operator<<(std::unique_ptr<Packet> &p,
                                    const NamedDataChunk &msg);
bool operator>>(std::unique_ptr<Packet> &p, NamedDataChunk &msg);



std::ostream &operator<<(std::ostream &stream, Packet &packet);

} // namespace MediaNet
