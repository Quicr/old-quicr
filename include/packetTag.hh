#pragma once

namespace MediaNet {

enum struct PacketTag : uint32_t;

constexpr uint32_t packetTagGen(uint16_t val,  int len,
                                    bool mandToUnderstand) {
  (void)len;
  uint32_t ret = val;
  ret <<= 8;
  ret += (mandToUnderstand?1:0);
  return ret;
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

  sync = packetTagGen(1, -1, true),
  syncAck = packetTagGen(2, -1, true),
  reset = packetTagGen(3, -1, true),
  resetRetry = packetTagGen(4, -1, true),
  resetRedirect = packetTagGen(5, -1, true),

  subscribe = packetTagGen(6, -1, true),
  clientData = packetTagGen(7, -1, true),
  nack = packetTagGen(8, -1, true),
  rate = packetTagGen(9, -1, true),
  ack = packetTagGen(10, -1, true),
  relayData = packetTagGen(11, -1, true),

  shortName = packetTagGen(12, -1, true),
  dataBlock = packetTagGen(12, -1, true),
  encDataBlock = packetTagGen(14, -1, true),

  subData = packetTagGen(50, -1, true), // TODO remove
  pubData = packetTagGen(51, -1, true), // TODO remove

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

  // extraMagicVer1 = packetTagGen(12538, 0, false),

  badTag = packetTagGen(16383, 0,
                        true), // must not have any tag values greater than this
};

}