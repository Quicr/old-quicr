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
  std::unique_ptr<Packet> recv() override;

    void updateMTU(uint16_t mtu) override;

private:
  RateCtrl rateCtrl;

  bool shutDown;

  void runNetRecv();
  std::thread recvThread;

  void runNetSend();
  std::thread sendThread;

  uint32_t oldPhase;

    uint16_t mtu;
};

} // namespace MediaNet
