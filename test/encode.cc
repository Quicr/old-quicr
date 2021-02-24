#include <doctest/doctest.h>
#include <memory>
#include <iostream>

#include "encode.hh"
#include "packet.hh"

using namespace MediaNet;

///
/// Atomic Types encode/decode tests
///

TEST_CASE("string encode/decode") {
	std::string origin_in = "This is a string with : , * () + - !";
	auto packet = std::make_unique<Packet>();
  std::cout << packet->to_hex() << std::endl;
  std::string origin_out;
  packet >> origin_out;
  CHECK_EQ(origin_in, origin_in);
}


///
/// Protocol messages encode/decode
///
// TODO: match hex encoding

TEST_CASE("NetSyncReq encode/decode") {
	NetSyncReq req_in;
	req_in.cookie = 0xC001E;
	req_in.origin = "example.com";
	req_in.senderId = 0x1234;
	req_in.supportedFeaturesVec = 0xABCD;
	req_in.clientTimeMs = 0xA1B1C1D1;

	auto packet = std::make_unique<Packet>();
	packet << req_in;

  NetSyncReq req_out;
  packet >> req_out;

	REQUIRE(req_in.cookie == req_out.cookie);
	REQUIRE(req_in.origin == req_out.origin);
	REQUIRE(req_in.senderId == req_out.senderId);
	REQUIRE(req_in.supportedFeaturesVec == req_out.supportedFeaturesVec);
	REQUIRE(req_in.clientTimeMs == req_out.clientTimeMs);
}

TEST_CASE("NetSyncAck encode/decode") {
	NetSyncAck ack_in;
	ack_in.useFeaturesVec = 0x1111;
	ack_in.serverTimeMs = 0x2222;

	auto packet = std::make_unique<Packet>();
	packet << ack_in;

	NetSyncAck ack_out;
	packet >> ack_out;

	CHECK_EQ(ack_in.serverTimeMs, ack_out.serverTimeMs);
	CHECK_EQ(ack_in.useFeaturesVec, ack_out.useFeaturesVec);
}

TEST_CASE("NetRstRetry encode/decode") {
	NetRstRetry retry_in;
	retry_in.cookie = 0x1234;

	auto packet = std::make_unique<Packet>();
	packet << retry_in;

	NetRstRetry retry_out;
	packet >> retry_out;

	CHECK_EQ(retry_in.cookie, retry_out.cookie);
}

TEST_CASE("NetRstRedirect encode/decode") {
	NetRstRedirect redirect_in;
	redirect_in.port = 0x1000;
	redirect_in.origin = "example.com";
	redirect_in.cookie = 0x1234;

	auto packet = std::make_unique<Packet>();
	packet << redirect_in;

	NetRstRedirect redirect_out;
	packet >> redirect_out;

	CHECK_EQ(redirect_in.cookie, redirect_out.cookie);
	CHECK_EQ(redirect_in.origin, redirect_out.origin);
	CHECK_EQ(redirect_in.port, redirect_out.port);
}