#pragma once

#include <cstdint>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <variant>
#include <optional>

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

class ConnectionPipe : public PipeInterface {
public:
  explicit ConnectionPipe(PipeInterface *t);
  void setAuthInfo(uint32_t sender, uint64_t t);
  bool start(uint16_t port, std::string server,
             PipeInterface *upStream) override;
  bool ready() const override;
  void stop() override;

  std::unique_ptr<Packet> recv() override;

private:
	//             +------> Start
	//             |          |
	//       fail/ |          | sync
	//       Rst   |          V
	//             +---- ConnectionPending
	//             |          |
	//             |          | syncAck
	//             |          V
	//             +------ Connected <----+
	//  											|						| syn/synAck
	//  											|						|
	//  											+------------

	struct Start {};
	struct ConnectionPending {};
	struct Connected {};
	using State = std::variant<Start, ConnectionPending, Connected>;

	static constexpr int syn_timeout_msec = 1000;
	static constexpr int connection_setup_timeout_msec = 5000;
	static constexpr int max_connection_retry_cnt = 5;
	void runSyncLoop();
	void sendSync();

	uint8_t syncs_awaiting_response = 0;
	uint32_t senderID;
  uint64_t token;
  uint64_t cookie = 0;
  bool open = false; // use state
  State state = State{};
  bool syncLoopRunning = false;
};

} // namespace MediaNet