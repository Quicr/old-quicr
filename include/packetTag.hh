#pragma once

namespace MediaNet {

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

  sync = packetTagGen(4, 255, true),
  syncAck = packetTagGen(10, 255, true),
  rstRetry = packetTagGen(11, 255, true),
  rstRedirect = packetTagGen(12, 255, true),

  nack = packetTagGen(14, 255, true),
  relayRateReq = packetTagGen(6, 4, true),
  ack = packetTagGen(3, 255, true),
  subscribeReq = packetTagGen(8, 0, true),

  clientData = packetTagGen(2, 4, true),
  relayData = packetTagGen(7, 8, true),
  pubData = packetTagGen(1, 255, true),
  // pubDataFrag = packetTagGen(9, 255, true),
  subData = packetTagGen(13, 0, true),

  shortName = packetTagGen(5, 18, true),

  // shortName = packetTagGen(5, 18, true),

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