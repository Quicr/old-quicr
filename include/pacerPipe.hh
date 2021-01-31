#pragma once

#include <cstdint>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

#include "packet.hh"
#include "pipeInterface.hh"
#include "rateCtrl.hh"

namespace MediaNet {

class PacerPipe : public PipeInterface {
public:
  explicit PacerPipe(PipeInterface *t);
  ~PacerPipe() override;

  bool start(uint16_t port, std::string server,
                     PipeInterface *upStream) override;
  [[nodiscard]] bool ready() const override;
  void stop() override;

  uint64_t getTargetUpstreamBitrate(); // in bps

  bool send(std::unique_ptr<Packet>) override;

  /// non blocking, return nullptr if no buffer
  std::unique_ptr<Packet> recv() override;

private:
  RateCtrl rateCtrl;

  bool shutDown;

  void runNetRecv();
  std::queue<std::unique_ptr<Packet>> recvQ;
  std::mutex recvQMutex;
  std::thread recvThread;

  void runNetSend();
  std::queue<std::unique_ptr<Packet>> sendQ;
  std::mutex sendQMutex;
  std::thread sendThread;

  uint32_t oldPhase;
};

} // namespace MediaNet
