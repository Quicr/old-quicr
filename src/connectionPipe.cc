

#include <cassert>
#include <thread>

#include "connectionPipe.hh"
#include "encode.hh"
#include "packet.hh"

using namespace MediaNet;

ConnectionPipe::ConnectionPipe(PipeInterface *t)
    : PipeInterface(t), senderID(0), token(0), open(false) {}

bool ConnectionPipe::start(const uint16_t port, const std::string server,
                           PipeInterface *upStrm) {
  bool ret = PipeInterface::start(port, server, upStrm);
  state = ConnectionPending{};
  syncConnection();
  return ret;
}

bool ConnectionPipe::ready() const {
  // TODO - wail until we have a synAck
	if(!std::holds_alternative<Connected>(state)) {
		return false;
	}
  return PipeInterface::ready();
}

void ConnectionPipe::stop() {
  open = false;

  // TODO send a Rst
  auto packet = std::make_unique<Packet>();
  assert(packet);
  // packet->buffer.reserve(20); // TODO - tune the 20

  packet << PacketTag::headerMagicRst;
  // packet << PacketTag::extraMagicVer1;
  // packet << PacketTag::extraMagicVer2;
  send(move(packet));

  PipeInterface::stop();
}

std::unique_ptr<Packet> ConnectionPipe::recv() {
	auto packet = PipeInterface::recv();
	if(packet == nullptr) {
		return packet;
	}

	// TODO: be careful when implementing the server side
	auto tag = nextTag(packet);
	if(tag != PacketTag::syncAck) {
		return packet;
	}

	// got syncAck
	state =  Connected{};
	NetSyncAck syncAck{};
	packet >> syncAck;
	// don't need to report syncAck up in the chain
	return nullptr;
}

void ConnectionPipe::setAuthInfo(uint32_t sender, uint64_t token_in) {
  senderID = sender;
  assert(senderID > 0);
  token = token_in;
}

void ConnectionPipe::syncConnection() {
	auto packet = std::make_unique<Packet>();
	assert(packet);
	packet << PacketTag::headerMagicSyn;

	NetSyncReq synReq{};
	synReq.clientTimeMs = 0; // TODO
	synReq.senderId = senderID;
	synReq.versionVec = 1;
	packet << synReq;

	send(move(packet));

	// resync timer
	auto resync_callback = [=]() {
		// check status and send a resync event
		syncConnection();
	};

	Timer::wait(syn_timeout_msec, std::move(resync_callback));
}