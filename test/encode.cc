#include <doctest/doctest.h>
#include <memory>

#include "encode.hh"
#include "name.hh"
#include "packet.hh"

using namespace MediaNet;

///
/// Atomic Types encode/decode tests
///

TEST_CASE("string encode/decode") {
  std::string originIn = "This is a string with : , * () + - !";
  auto packet = std::make_unique<Packet>();
  std::string originOut;
  packet >> originOut;
  CHECK_EQ(originIn, originIn);
}

TEST_CASE("ShortName encode/decode") {
  ShortName name{};
  name.resourceID = 0xAA;
  name.senderID = 0xBB;
  name.sourceID = 0xCC;
  name.mediaTime = 0xDD;
  name.fragmentID = 0xEE;

  auto packet = std::make_unique<Packet>();
  packet << name;

  ShortName nameOut{};
  packet >> nameOut;

  CHECK_EQ(name, nameOut);
}

TEST_CASE("message header encode/decode") {
	auto header_in = Packet::Header{PacketTag::headerSyn, 1};
	auto packet = std::make_unique<Packet>();
	packet << header_in;
	auto token = packet->getPathToken();
	REQUIRE_EQ(token, header_in.pathToken);
	packet->setPathToken(0x2000);
	token = packet->getPathToken();
	REQUIRE_EQ(token, 0x2000);

	Packet::Header header_out{};
	packet >> header_out;
  REQUIRE_EQ(header_out.pathToken, 0x2000);
  REQUIRE_EQ(header_in.tag, header_out.tag);
}

TEST_CASE("RelayData encode/decode") {
  auto packet = std::make_unique<Packet>();

  RelayData dataIn{};
  dataIn.relaySeqNum = 0x1000;
  dataIn.relaySendTimeUs = 0;

  packet << dataIn;

  RelayData dataOut{};
  packet >> dataOut;

  CHECK_EQ(dataIn.relaySendTimeUs, dataOut.relaySendTimeUs);
  CHECK_EQ(dataIn.relaySeqNum, dataOut.relaySeqNum);
}

TEST_CASE("ClientData encode/decode") {
  auto packet = std::make_unique<Packet>();

  ClientData dataIn{};
  dataIn.clientSeqNum = 0x1000;

  packet << dataIn;

  ClientData dataOut{};
  packet >> dataOut;

  CHECK_EQ(dataIn.clientSeqNum, dataOut.clientSeqNum);
}

///
/// Protocol messages encode/decode
///
// TODO: match hex encoding

TEST_CASE("NetSyncReq encode/decode") {
  NetSyncReq reqIn;
  reqIn.cookie = 0xC001E;
  reqIn.origin = "example.com";
  reqIn.senderId = 0x1234;
  reqIn.supportedFeaturesVec = 0xABCD;
  reqIn.clientTimeMs = 0xA1B1C1D1;

  auto packet = std::make_unique<Packet>();
  packet << reqIn;

  NetSyncReq reqOut;
  packet >> reqOut;

  REQUIRE(reqIn.cookie == reqOut.cookie);
  REQUIRE(reqIn.origin == reqOut.origin);
  REQUIRE(reqIn.senderId == reqOut.senderId);
  REQUIRE(reqIn.supportedFeaturesVec == reqOut.supportedFeaturesVec);
  REQUIRE(reqIn.clientTimeMs == reqOut.clientTimeMs);
}

TEST_CASE("NetSyncAck encode/decode") {
  NetSyncAck ackIn;
  ackIn.useFeaturesVec = 0x1111;
  ackIn.serverTimeMs = 0x2222;

  auto packet = std::make_unique<Packet>();
  packet << ackIn;

  NetSyncAck ackOut;
  packet >> ackOut;

  CHECK_EQ(ackIn.serverTimeMs, ackOut.serverTimeMs);
  CHECK_EQ(ackIn.useFeaturesVec, ackOut.useFeaturesVec);
}

TEST_CASE("NetResetRetry encode/decode") {
  NetResetRetry retryIn;
  retryIn.cookie = 0x1234;

  auto packet = std::make_unique<Packet>();
  packet << retryIn;

  NetResetRetry retryOut;
  packet >> retryOut;

  CHECK_EQ(retryIn.cookie, retryOut.cookie);
}

TEST_CASE("NetResetRedirect encode/decode") {
  NetResetRedirect redirectIn;
  redirectIn.port = 0x1000;
  redirectIn.origin = "example.com";
  redirectIn.cookie = 0x1234;

  auto packet = std::make_unique<Packet>();
  packet << redirectIn;

  NetResetRedirect redirectOut;
  packet >> redirectOut;

  CHECK_EQ(redirectIn.cookie, redirectOut.cookie);
  CHECK_EQ(redirectIn.origin, redirectOut.origin);
  CHECK_EQ(redirectIn.port, redirectOut.port);
}

TEST_CASE("NetRateReq encode/decode") {
  NetRateReq reqIn;
  reqIn.bitrateKbps = toVarInt(0x1000);

  auto packet = std::make_unique<Packet>();
  packet << reqIn;

  NetRateReq reqOut;
  packet >> reqOut;

  CHECK_EQ(reqIn.bitrateKbps, reqOut.bitrateKbps);
}

TEST_CASE("NetAck encode/decode") {
  NetAck ackIn;
  ackIn.ecnVec = 0x1;
  ackIn.ackVec = 0x4;
  ackIn.clientSeqNum = 0x1000;
  ackIn.recvTimeUs = 0x2000;

  auto packet = std::make_unique<Packet>();
  packet << ackIn;

  NetAck ackOut;
  packet >> ackOut;

  CHECK_EQ(ackIn.ecnVec, ackOut.ecnVec);
  CHECK_EQ(ackIn.ackVec, ackOut.ackVec);
  CHECK_EQ(ackIn.recvTimeUs, ackOut.recvTimeUs);
  CHECK_EQ(ackIn.clientSeqNum, ackOut.clientSeqNum);
}


TEST_CASE("ClientData encode/decode") {
  auto dataIn = std::array<uint8_t,6>{0x1, 0x2, 0x3, 0x4, 0x5, 0xA};
  ClientData clientDataIn;
  NamedDataChunk chunkIn;
  DataBlock dataBlockIn;

  clientDataIn.clientSeqNum= 23;

  chunkIn.shortName = ShortName(1, 2, 3);
  chunkIn.lifetime = toVarInt(0x1000);

  dataBlockIn.metaDataLen = toVarInt( 0 );
  dataBlockIn.dataLen = toVarInt( dataIn.size() );

  auto packet = std::make_unique<Packet>();
  packet << dataIn;
  packet << dataBlockIn;
  packet << chunkIn;
  packet << clientDataIn;

  ClientData clientDataOut;
  NamedDataChunk chunkOut;
  DataBlock dataBlockOut;
  std::array<uint8_t,dataIn.size()> dataOut;

  packet >> clientDataOut;
  packet >> chunkOut;
  packet >> dataBlockOut;
  packet >> dataOut;

  CHECK_EQ(clientDataIn.clientSeqNum, clientDataOut.clientSeqNum);
  CHECK(chunkIn.shortName == chunkOut.shortName);
  CHECK_EQ(chunkIn.lifetime, chunkOut.lifetime);
  CHECK_EQ(dataBlockIn.metaDataLen,dataBlockOut.metaDataLen);
  CHECK_EQ(dataBlockIn.dataLen,dataBlockOut.dataLen);
  CHECK_EQ( dataIn[0], dataOut[0] );
  CHECK_EQ( dataIn, dataOut );
}


TEST_CASE("RelayData encode/decode") {
  auto dataIn = std::vector<uint8_t>{0x1, 0x2, 0x3, 0x4, 0x5, 0xA};
  RelayData relayDataIn;
  NamedDataChunk chunkIn;
  EncryptedDataBlock dataBlockIn;

  relayDataIn.relaySeqNum= 23;

  chunkIn.shortName = ShortName(1, 2, 3);
  chunkIn.lifetime = toVarInt(0x1000);

  dataBlockIn.metaDataLen = toVarInt( 0 );
  dataBlockIn.authTagLen = 4;
  dataBlockIn.cipherDataLen = toVarInt( dataIn.size() );

  auto packet = std::make_unique<Packet>();
  packet << dataIn;
  packet << dataBlockIn;
  packet << chunkIn;
  packet << relayDataIn;

  RelayData relayDataOut;
  NamedDataChunk chunkOut;
  EncryptedDataBlock dataBlockOut;

  packet >> relayDataOut;
  packet >> chunkOut;
  packet >> dataBlockOut;

  CHECK_EQ(relayDataIn.relaySeqNum, relayDataOut.relaySeqNum);
  CHECK(chunkIn.shortName == chunkOut.shortName);
  CHECK_EQ(chunkIn.lifetime, chunkOut.lifetime);
  CHECK_EQ(dataBlockIn.metaDataLen,dataBlockOut.metaDataLen);
  CHECK_EQ(dataBlockIn.authTagLen,dataBlockOut.authTagLen);
  CHECK_EQ(dataBlockIn.cipherDataLen,dataBlockOut.cipherDataLen);
}
