#pragma once

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
#include <vector>

namespace MediaNet {

class UdpPipe;
class FecPipe;
class QRelay;
class QuicRClient;

struct IpAddr {
  struct sockaddr_in addr;
  socklen_t addrLen;

  static std::string toString(const IpAddr &);
  bool operator<(const IpAddr &rhs) const;
};

class Packet {
  friend UdpPipe;
  friend FecPipe;
  friend QRelay;
  friend QuicRClient;

public:
  struct ShortName {
    uint64_t resourceID;
    uint32_t senderID;
    uint8_t sourceID;
    uint32_t mediaTime;
    uint8_t fragmentID;
  };

  Packet();
  void copy(const Packet &p);

  // void push( PacketTag tag, const std::vector<uint8_t> value );
  // PacketTag peek();
  // std::vector<uint8_t> pop( PacketTag tag );

  uint8_t &data() { return buffer.at(headerSize); }
  int size() { return buffer.size() - headerSize; }
  void resize(int size) { buffer.resize(headerSize + size); }

  std::vector<uint8_t> buffer;

  void setReliable(bool reliable = true);
  void enableFEC(bool doFec = true);

  [[nodiscard]] const IpAddr &getSrc() const;
  void setSrc(const IpAddr &src);
  [[nodiscard]] const IpAddr &getDst() const;
  void setDst(const IpAddr &dst);

  [[nodiscard]] std::unique_ptr<Packet> clone() const;

  [[nodiscard]] const ShortName shortName() const { return name; };

private:
  MediaNet::Packet::ShortName name;
  int headerSize;

  // 1 is higest (controll), 2 critical audio, 3 critical
  // video, 4 important, 5 not important - make enum
  uint8_t priority;

  bool reliable;
  bool useFEC;

  MediaNet::IpAddr src;
  MediaNet::IpAddr dst;
};

} // namespace MediaNet
