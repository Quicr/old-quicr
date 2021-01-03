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
  PacerPipe(PipeInterface *t);
  ~PacerPipe();

  virtual bool start(const uint16_t port, const std::string server,
                     PipeInterface *upStream);
  virtual bool ready() const;
  virtual void stop();

  uint64_t getTargetUpstreamBirate(); // in bps

  virtual bool send(std::unique_ptr<Packet>);

  /// non blocking, return nullptr if no buffer
  virtual std::unique_ptr<Packet> recv();

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
