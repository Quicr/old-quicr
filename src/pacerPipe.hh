#pragma once

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

#include "pipeInterface.hh"
#include "quicr/packet.hh"
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

  bool send(std::unique_ptr<Packet>) override;
  std::unique_ptr<Packet> recv() override;

  uint64_t getTargetUpstreamBitrate(); // in bps

  void updateMTU(uint16_t mtu, uint32_t pps) override;
  void updateBitrateUp(uint64_t minBps, uint64_t startBps,
                       uint64_t maxBps) override;
  void updateRTT(uint16_t minRttMs, uint16_t bigRttMs) override;

private:
  RateCtrl rateCtrl;

  bool shutDown;

  void runNetRecv();
  std::thread recvThread;

  void runNetSend();
  std::thread sendThread;

  void sendRateCommand();

  uint32_t oldPhase;
  std::chrono::steady_clock::time_point phaseStartTime;
  uint32_t packetsSentThisPhase;

  uint16_t mtu;
  uint32_t targetPpsUp;
  bool useConstantPacketRate;

  std::atomic<uint32_t> nextSeqNum; // TODO - may not need atomic here
};

} // namespace MediaNet
