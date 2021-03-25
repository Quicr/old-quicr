#pragma once

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <random>
#include <string>
#include <variant>

#include "packet.hh"
#include "pipeInterface.hh"

namespace MediaNet {

///
/// ConnectionPipe
///
class ConnectionPipe : public PipeInterface {
public:
  explicit ConnectionPipe(PipeInterface *t);
  bool ready() const override;
  void stop() override;
  virtual bool start(uint16_t port, const std::string& server,
                     PipeInterface *upStream) override;

protected:
  //             +------> Start
  //             |          |
  //       fail/ |          | sync
  //       Rst   |          V
  //             +---- ConnectionPending
  //             |         ^    |
  //             |    syn  |    | syncAck
  //             |         |    V
  //             +------ Connected
  //

  struct Start {};
  struct ConnectionPending {};
  struct Connected {};
  using State = std::variant<Start, ConnectionPending, Connected>;

  State state = Start{};
};

///
/// ClientConnectionPipe
///
class ClientConnectionPipe : public ConnectionPipe {
public:
  explicit ClientConnectionPipe(PipeInterface *t);
  void setAuthInfo(uint32_t sender, uint64_t t);
  bool start(uint16_t port, const std::string& server,
             PipeInterface *upStream) override;

  // Overrides from PipelineInterface
  bool send(std::unique_ptr<Packet>) override;
  std::unique_ptr<Packet> recv() override;
	void runUpdates(const std::chrono::time_point<std::chrono::steady_clock>& now) override;

private:
  static constexpr int syn_timeout_msec = 1000;
  static constexpr int max_connection_retry_cnt = 5;

  void sendSync();

  uint8_t syncs_awaiting_response = 0;
  uint32_t senderID;
  uint32_t pathToken = 0;
  uint64_t cookie = 0;
	std::chrono::time_point<std::chrono::steady_clock> last_sync_point;
};

///
/// ServerConnectionPipe
///

// TODO: Add client connection status loop
class ServerConnectionPipe : public ConnectionPipe {
  using timepoint = std::chrono::time_point<std::chrono::steady_clock>;

public:
  class Connection {
  public:
    Connection(uint32_t relaySeq, uint64_t cookie_in);
    uint32_t relaySeqNum;
    uint64_t cookie;
    timepoint lastSyn;
  };

  explicit ServerConnectionPipe(PipeInterface *t);
  bool start(uint16_t port, const std::string& server,
             PipeInterface *upStream) override;
  // Overrides from PipelineInterface
  bool send(std::unique_ptr<Packet>) override;
  std::unique_ptr<Packet> recv() override;

private:
  void processSyn(std::unique_ptr<MediaNet::Packet> &packet);
  void processRst(std::unique_ptr<MediaNet::Packet> &packet);
  void sendSyncAck(const MediaNet::IpAddr &to, uint32_t authSecret);

  // TODO: need to timeout on the entries in this map to
  // avoid DOS attacks
  std::map<MediaNet::IpAddr, std::tuple<timepoint, uint32_t>> cookies;
  std::map<MediaNet::IpAddr, std::unique_ptr<Connection>> connectionMap;
  std::map<MediaNet::IpAddr, uint32_t> pathTokens;

  // TODO revisit this (use cryptographic random)
  std::mt19937 randomGen;
  std::uniform_int_distribution<uint32_t> randomDist;
  std::function<uint32_t()> getRandom;
};

} // namespace MediaNet