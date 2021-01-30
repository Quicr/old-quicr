#pragma once

#include <sys/types.h>
#include <mutex>

#if defined(__linux__) || defined(__APPLE__)
#include <netinet/in.h>
#include <sys/socket.h>
#elif defined(_WIN32)
#include <WinSock2.h>
#include <ws2tcpip.h>
#endif

#include <cstdint>
#include <string>

#include "packet.hh"
#include "pipeInterface.hh"

namespace MediaNet {

class UdpPipe : public PipeInterface {
public:
  UdpPipe();
  ~UdpPipe();

  virtual bool start(const uint16_t serverPort, const std::string serverName,
                     PipeInterface *upStream);
  virtual bool ready() const;
  virtual void stop();

  virtual bool send(std::unique_ptr<Packet>);
  virtual std::unique_ptr<Packet> recv(); // non blocking, return nullptr if no buffer

private:
    std::mutex socketMutex;
#if defined(_WIN32)
  SOCKET fd; // UDP socket
#else
  int fd; // UDP socket
#endif

  IpAddr serverAddr;
};

bool operator<(const ShortName &a, const ShortName &b);


} // namespace MediaNet
