#include <cassert>
#include <random>
#include <thread>

#include "connectionPipe.hh"
#include "encode.hh"
#include "packet.hh"

using namespace MediaNet;

ConnectionPipe::ConnectionPipe(PipeInterface *t) : PipeInterface(t) {}

bool ConnectionPipe::start(const uint16_t port, const std::string server,
                           PipeInterface *upStrm) {
  return PipeInterface::start(port, server, upStrm);
}

bool ConnectionPipe::ready() const {
  if (!std::holds_alternative<Connected>(state)) {
    return false;
  }
  return PipeInterface::ready();
}

void ConnectionPipe::stop() {
  state = Start{};
  auto packet = std::make_unique<Packet>();
  assert(packet);
  packet << PacketTag::headerRst;
  std::clog << "ConnectionPipe:Stop: Reset: " << packet->to_hex() << std::endl;
  send(move(packet));

  PipeInterface::stop();
}

///
/// ClientConnectionPipe
///

ClientConnectionPipe::ClientConnectionPipe(PipeInterface *t)
    : ConnectionPipe(t), senderID(0), pathToken(0) {}

bool ClientConnectionPipe::start(uint16_t port, std::string server,
                                 PipeInterface *upStream) {
  bool ret = ConnectionPipe::start(port, server, upStream);
  if (ret) {
    sendSync();
    runSyncLoop();
  }
  return ret;
}

bool ClientConnectionPipe::send(std::unique_ptr<Packet> packet) {
	// insert pathToken in the header
	return send(move(packet));
}

std::unique_ptr<Packet> ClientConnectionPipe::recv() {
  auto packet = PipeInterface::recv();
  if (packet == nullptr) {
    return packet;
  }

  auto tag = nextTag(packet);
  if (tag == PacketTag::syncAck) {
  	std::clog << "ConnectionPipe: Got syncAck" <<  std::endl;
  	NetSyncAck syncAck{};
    packet >> syncAck;

    // kickoff periodic sync flow
    if (!syncLoopRunning) {
      runSyncLoop();
      syncLoopRunning = true;
    }
    state = Connected{};
    // don't need to report syncAck up in the chain
    return nullptr;
  } else if (tag == PacketTag::resetRetry) {
    NetResetRetry rstRetry{};
    packet >> rstRetry;
    std::clog << "ConnectionPipe: Got resetRetry: cookie " << rstRetry.cookie
              << std::endl;
    cookie = rstRetry.cookie;
    sendSync();
    return nullptr;
  }

  return packet;
}

void ClientConnectionPipe::setAuthInfo(uint32_t sender, uint64_t token_in) {
  senderID = sender;
  assert(senderID > 0);
  pathToken = token_in;
}

void ClientConnectionPipe::sendSync() {
  auto packet = std::make_unique<Packet>();
  assert(packet);
  auto header = Packet::Header{PacketTag::headerSyn, pathToken};
  packet << header;
  NetSyncReq synReq{};
  const auto now = std::chrono::system_clock::now();
  const auto duration = now.time_since_epoch();
  synReq.clientTimeMs =
      std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
  ;
  synReq.senderId = senderID;
  synReq.supportedFeaturesVec = 1;
  synReq.cookie = cookie;
  synReq.origin = "example.com";
  // std::clog << "syncConnection: cookie:" << synReq.cookie << std::endl;
  packet << synReq;
  // std::clog <<"sync Packet: " << packet->to_hex() << std::endl;
  send(move(packet));
  state = ConnectionPending{};
}

void ClientConnectionPipe::runSyncLoop() {
  if (!syncLoopRunning) {
    syncLoopRunning = true;
  }

  auto resync_callback = [=]() {
    // check status and send a resync event
    if (std::holds_alternative<ConnectionPending>(state)) {
      if (syncs_awaiting_response >= max_connection_retry_cnt) {
        stop();
        return;
      }
      syncs_awaiting_response++;
    }
    runSyncLoop();
  };

  sendSync();
  Timer::wait(syn_timeout_msec, std::move(resync_callback));
}

///
/// ServerConnectionPipe
///
bool dont_send_sync_ack = false;

ServerConnectionPipe::Connection::Connection(uint32_t relaySeq,
                                             uint64_t cookie_in)
    : relaySeqNum(relaySeq), cookie(cookie_in) {}

ServerConnectionPipe::ServerConnectionPipe(PipeInterface *t)
    : ConnectionPipe(t) {
  std::random_device randDev;
  randomGen.seed(randDev()); // TODO - should use crypto random
  getRandom = std::bind(randomDist, randomGen);
}

bool ServerConnectionPipe::start(uint16_t port, std::string server,
                                 PipeInterface *upStream) {
  bool ret = ConnectionPipe::start(port, server, upStream);
  if (ret) {
    state = Connected{};
  }
  return ret;
}

std::unique_ptr<Packet> ServerConnectionPipe::recv() {
  auto packet = PipeInterface::recv();
  if (packet == nullptr) {
    return packet;
  }

  auto tag = nextTag(packet);

  if (tag == PacketTag::sync) {
    processSyn(packet);
    return nullptr;
  } else if (tag == PacketTag::headerRst) {
    processRst(packet);
    return nullptr;
  }

  // let the app handle this packet
  return packet;
}

void ServerConnectionPipe::processSyn(
    std::unique_ptr<MediaNet::Packet> &packet) {
  std::clog << "Got a Syn"
            << " from=" << IpAddr::toString(packet->getSrc())
            << " len=" << packet->fullSize() << std::endl;

  NetSyncReq sync = {};
  packet >> sync;

  auto conIndex = connectionMap.find(packet->getSrc());
  if (conIndex != connectionMap.end()) {
    // existing connection
    std::unique_ptr<Connection> &con = connectionMap[packet->getSrc()];
    con->lastSyn = std::chrono::steady_clock::now();
    sendSyncAck(packet->getSrc(), {});
    return;
  }

  // new connection or retry with cookie
  auto it = cookies.find(packet->getSrc());
  if (it == cookies.end()) {
    // send a reset with retry cookie
    auto cookie = random();
    cookies.emplace(packet->getSrc(),
                    std::make_tuple(std::chrono::steady_clock::now(), cookie));

    auto rstPkt = std::make_unique<Packet>();
    NetResetRetry rstRetry{};
    rstRetry.cookie = cookie;
    rstPkt << PacketTag::headerRst;
    rstPkt << rstRetry;
    rstPkt->setDst(packet->getSrc());
    std::clog << "new connection attempt, generate cookie:" << cookie << std::endl;
    send(std::move(rstPkt));
    return;
  }

  // cookie exists, verify it matches
  auto &[when, cookie] = it->second;
  // TODO: verify now() - when is within in acceptable limits
  if (sync.cookie != cookie) {
    // bad cookie, reset the connection
    auto rstPkt = std::make_unique<Packet>();
    rstPkt << PacketTag::headerRst;
    rstPkt->setDst(packet->getSrc());
    std::clog << "incorrect cookie: found:" << sync.cookie << ", expected:" << cookie << std::endl;
		send(std::move(rstPkt));
    return;
  }

  // good sync new connection
  connectionMap[packet->getSrc()] =
      std::make_unique<Connection>(getRandom(), cookie);
  cookies.erase(it);
  std::clog << "Added connection:"
            << MediaNet::IpAddr::toString(packet->getSrc()) << std::endl;
  sendSyncAck(packet->getSrc(), {});
}

void ServerConnectionPipe::processRst(
    std::unique_ptr<MediaNet::Packet> &packet) {
  auto conIndex = connectionMap.find(packet->getSrc());
  if (conIndex == connectionMap.end()) {
    std::clog << "Reset receieved for unknown connection\n";
    return;
  }
  connectionMap.erase(conIndex);
  std::clog << "Reset recieved for connection: "
            << IpAddr::toString(packet->getSrc()) << "\n";
}

void ServerConnectionPipe::sendSyncAck(const MediaNet::IpAddr &to,
                                       uint64_t authSecret) {
  (void)authSecret; // TODO
  if (dont_send_sync_ack) {
    std::clog << "Server not sending syn-ack\n";
    return;
  }
  auto syncAckPkt = std::make_unique<Packet>();
  auto syncAck = NetSyncAck{};
  const auto now = std::chrono::system_clock::now();
  const auto duration = now.time_since_epoch();
  syncAck.serverTimeMs =
      std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
  auto header = Packet::Header{PacketTag::headerSynAck, authSecret};
  syncAckPkt << header;
  syncAckPkt << syncAck;
  syncAckPkt->setDst(to);
  // std::clog << "SyncAck: " << syncAckPkt->to_hex() << std::endl;
  send(std::move(syncAckPkt));
}
