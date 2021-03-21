#pragma once

#include <mutex>
#include <sys/types.h>

#if defined(__linux__) || defined(__APPLE__)
#include <netinet/in.h>
#include <sys/socket.h>
#elif defined(_WIN32)
#include <WinSock2.h>
#include <ws2tcpip.h>
#endif

#include <cstdint>
#include <string>

#include "pipeInterface.hh"
#include "quicr/packet.hh"

namespace MediaNet {

class UdpPipe : public PipeInterface {
public:
  UdpPipe();
  ~UdpPipe() override;

  bool start(uint16_t serverPort, std::string serverName,
             PipeInterface *upStream) override;
  [[nodiscard]] bool ready() const override;
  void stop() override;

  bool send(std::unique_ptr<Packet>) override;
  std::unique_ptr<Packet>
  recv() override; // non blocking, return nullptr if no buffer

private:
  std::mutex socketMutex;
#if defined(_WIN32)
  SOCKET fd; // UDP socket
#else
  int fd; // UDP socket
#endif

  IpAddr serverAddr;
};

} // namespace MediaNet
