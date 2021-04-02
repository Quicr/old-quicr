#pragma once

namespace MediaNet {

enum struct PacketTag : uint32_t;

constexpr uint32_t
packetTagGen(uint16_t val, int len, bool mandToUnderstand)
{
  (void)len;
  uint32_t ret = val;
  ret <<= 8;
  ret += (mandToUnderstand ? 1 : 0);
  return ret;
}

constexpr uint16_t
packetTagTrunc(MediaNet::PacketTag tag)
{
  auto t = (uint32_t)tag;
  t >>= 8;
  return (uint16_t)t;
}

enum struct PacketTag : uint32_t
{
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
  dataBlock = packetTagGen(13, -1, true),
  encDataBlock = packetTagGen(14, -1, true),
  header = packetTagGen(15, 0, true), // tag for the header itself

  // This block of headerMagic values selected to multiplex with STUN/DTLS/RTP
  headerData = packetTagGen(80, 0, true),
  headerDataCrazy = packetTagGen(81, 0, true),
  headerSyn = packetTagGen(82, 0, true),
  headerSynCrazy = packetTagGen(83, 0, true),
  headerSynAck = packetTagGen(84, 0, true),
  headerSynAckCrazy = packetTagGen(85, 0, true),
  headerRst = packetTagGen(86, 0, true),
  headerRstCrazy = packetTagGen(87, 0, true),

  badTag = packetTagGen(16383,
                        0,
                        true), // must not have any tag values greater than this
};

}