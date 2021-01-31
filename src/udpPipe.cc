
#include <cassert>
#include <iostream>
#include <thread>

#if defined(__linux) || defined(__APPLE__)
#include <arpa/inet.h>
#include <netdb.h>
#endif
#if defined(__linux__)
#include <net/ethernet.h>
#include <netpacket/packet.h>
#elif defined(__APPLE__)
#include <net/if_dl.h>
#include <unistd.h>
#elif defined(_WIN32)
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

#include "packet.hh"
#include "udpPipe.hh"

using namespace MediaNet;

UdpPipe::UdpPipe() : PipeInterface(nullptr),serverAddr() { fd = 0; }

UdpPipe::~UdpPipe() {
    if (fd > 0) {
        std::lock_guard<std::mutex> lock(socketMutex);
#if defined(_WIN32)
        closesocket(fd);
#else
        ::close(fd);
#endif
        fd = 0;
    }
}

bool UdpPipe::ready() const { return (fd > 0); }

void UdpPipe::stop() {
  if (fd > 0) {
    std::lock_guard<std::mutex> lock(socketMutex);
#if defined(_WIN32)
    closesocket(fd);
#else
    ::close(fd);
#endif
    fd = 0;
  }
}

bool UdpPipe::send(std::unique_ptr<Packet> packet) {
  // std::lock_guard<std::mutex> lock(socketMutex);

  if (fd == 0) {
    return false;
  }

  if (!packet) {
    return false;
  }

  if (packet->fullSize() == 0) {
    return false;
  }

  IpAddr addr = serverAddr;
  if (packet->dst.addrLen != 0) {
    addr = packet->dst;
  }
  // std::clog << "Send to " << addr.toString() << std::endl;

  int numSent = sendto(fd, (const char *)packet->buffer.data(),
                       (int)packet->buffer.size(), 0 /*flags*/,
                       (struct sockaddr *)&(addr.addr), addr.addrLen);
  if (numSent < 0) {
#if defined(_WIN32)
    int error = WSAGetLastError();
    if (error == WSAETIMEDOUT) {
      return false;
    } else {
      std::cerr << "sending on UDP socket got error: " << WSAGetLastError()
                << std::endl;
      assert(0);
    }
#else
    // TODO: this drops packet on floor, we need a way to
    // requeue/resend
    int e = errno;
    std::cerr << "sending on UDP socket got error: " << strerror(e)
              << std::endl;
    assert(0); // TODO
#endif
  } else if (numSent != (int)packet->buffer.size()) {
    assert(0); // TODO
  }

  return true;
}

std::unique_ptr<Packet> UdpPipe::recv() {
  std::lock_guard<std::mutex> lock(socketMutex);

  if (fd == 0) {
    return std::unique_ptr<Packet>(nullptr);
  }

  auto packet = std::make_unique<Packet>();

  const int dataSize = 1500;
  packet->buffer.resize(dataSize);

  IpAddr remoteAddr{};
  memset(&remoteAddr.addr, 0, sizeof(remoteAddr.addr));
  remoteAddr.addrLen = sizeof(remoteAddr.addr);

  int rLen = recvfrom(fd, (char *)packet->buffer.data(),
                      (int)packet->buffer.size(), 0 /*flags*/,
                      (struct sockaddr *)&remoteAddr.addr, &remoteAddr.addrLen);
  if (rLen < 0) {
#if defined(_WIN32)
    int error = WSAGetLastError();
    switch (error) {
    case WSAEINTR:
    case WSAETIMEDOUT:
      return std::unique_ptr<Packet>(nullptr);

    case WSAECONNRESET:
      // TODO - probably indicates relay not running
      return std::unique_ptr<Packet>(nullptr);

    default:
      std::cerr << "reading from UDP socket got error: " << WSAGetLastError()
                << std::endl;
      assert(0);
      break;
    }
#else
    int e = errno;
    if (e == EAGAIN) {
      // timeout on read
      return std::unique_ptr<Packet>(nullptr);
    } else {
      if (fd != 0) {
        std::cerr << "reading from UDP socket got error: " << strerror(e)
                  << std::endl;
      }
      // assert(0); // TODO
      return std::unique_ptr<Packet>(nullptr);
    }
#endif
  }

  if (rLen == 0) {
    return std::unique_ptr<Packet>(nullptr);
  }

  packet->src = remoteAddr;
  packet->buffer.resize(rLen);

  return packet;
}

bool UdpPipe::start(const uint16_t serverPort, const std::string serverName,
                    PipeInterface *upTransport) {
  std::lock_guard<std::mutex> lock(socketMutex);
  upStream = upTransport;

  // set up network
#if defined(_WIN32)
  WSADATA wsaData;
  int wsa_err = WSAStartup(MAKEWORD(2, 2), &wsaData);
  if (wsa_err) {
    assert(0);
  }
#endif

  // get a socket
  fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (fd < 0) {
    assert(0); // TODO
  }

  // make socket non blocking IO
  struct timeval timeOut{};
  timeOut.tv_sec = 0;
  timeOut.tv_usec = 2000; // TODO 2 ms
  auto err = setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (const char *)&timeOut,
                        sizeof(timeOut));
  if (err) {
    assert(0); // TODO
  }

  if (serverName.length() == 0) {
    // ================= set up server ======================

    // set for re-use
    int one = 1;
    err = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const char *)&one,
                     sizeof(one));
    if (err != 0) {
      assert(0); // TODO
    }

    // set up address and bind
    memset((char *)&serverAddr.addr, 0, sizeof(serverAddr.addr));
    serverAddr.addrLen = sizeof(serverAddr.addr);
    serverAddr.addr.sin_port = htons(serverPort);
    serverAddr.addr.sin_family = AF_INET;
    serverAddr.addr.sin_addr.s_addr = htonl(INADDR_ANY);

    err =
        bind(fd, (struct sockaddr *)&serverAddr.addr, sizeof(serverAddr.addr));
    if (err < 0) {
      assert(0); // TODO
    }

    std::cout << "UdpTransport: Server on port " << serverPort << ", fd " << fd
              << std::endl;
  } else {
    // ================= set up client  ======================

    struct sockaddr_in clientAddr{};
    clientAddr.sin_family = AF_INET;
    clientAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    clientAddr.sin_port = 0;
    err = bind(fd, (struct sockaddr *)&clientAddr, sizeof(clientAddr));
    if (err) {
      assert(0);
    }

    // find server address
    std::string sPort = std::to_string(htons(serverPort));
    struct addrinfo hints = {}, *address_list = nullptr;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;
    err = getaddrinfo(serverName.c_str(), sPort.c_str(), &hints, &address_list);
    if (err) {
      assert(0);
    }
    struct addrinfo *item = nullptr, *found_addr = nullptr;
    for (item = address_list; item != nullptr; item = item->ai_next) {
      if (item->ai_family == AF_INET && item->ai_socktype == SOCK_DGRAM &&
          item->ai_protocol == IPPROTO_UDP) {
        found_addr = item;
        break;
      }
    }
    if (found_addr == nullptr) {
      assert(0);
    }

    // setup server address
    memcpy(&serverAddr.addr, found_addr->ai_addr, found_addr->ai_addrlen);
    serverAddr.addr.sin_port = htons(serverPort);
    serverAddr.addrLen = sizeof(serverAddr.addr);

    std::cout << "UdpTransport: Client connect to " << serverName << ":"
              << serverPort << ", fd " << fd << std::endl;
  }

  return true;
}
