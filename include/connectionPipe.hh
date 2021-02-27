#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <thread>
#include <variant>
#include <optional>
#include <functional>
#include <random>
#include <map>

#include "packet.hh"
#include "pipeInterface.hh"

namespace MediaNet {

using WaitHandler = std::function<void()>;
struct Timer {
	static void wait(int msec, WaitHandler&& callback) {
		std::thread([=]() {
			auto wait_time = std::chrono::milliseconds(msec);
			std::this_thread::sleep_for(wait_time);
			callback();
		}).detach();
	}
};

///
/// ConnectionPipe
///
class ConnectionPipe : public PipeInterface {
public:
  explicit ConnectionPipe(PipeInterface *t);
	bool ready() const override;
	void stop() override;
	virtual bool start(uint16_t port, std::string server,
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
	bool start(uint16_t port, std::string server,
										 PipeInterface *upStream) override;

	std::unique_ptr<Packet> recv() override;

private:
	static constexpr int syn_timeout_msec = 1000;
	static constexpr int max_connection_retry_cnt = 5;

	void runSyncLoop();
	void sendSync();


	uint8_t syncs_awaiting_response = 0;
	uint32_t senderID;
	uint64_t token;
	uint64_t cookie = 0;
	bool syncLoopRunning = false;
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
		uint64_t  cookie;
		timepoint lastSyn;
	};

	explicit ServerConnectionPipe(PipeInterface *t);
	bool start(uint16_t port, std::string server,
						 PipeInterface *upStream) override;
	std::unique_ptr<Packet> recv() override;

private:

	void processSyn(std::unique_ptr<MediaNet::Packet>& packet);
	void processRst(std::unique_ptr<MediaNet::Packet>& packet);
	void sendSyncAck(const MediaNet::IpAddr& to, uint64_t authSecret);

	// TODO: need to timeout on the entries in this map to
	// avoid DOS attacks
	std::map<MediaNet::IpAddr, std::tuple<timepoint, uint32_t>> cookies;
	std::map<MediaNet::IpAddr, std::unique_ptr<Connection>> connectionMap;

	// TODO revisit this (use cryptographic random)
	std::mt19937 randomGen;
	std::uniform_int_distribution<uint32_t> randomDist;
	std::function<uint32_t()> getRandom;
};


} // namespace MediaNet