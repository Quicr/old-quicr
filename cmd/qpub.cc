

#include <cassert>
#include <chrono>
#include <iostream>
#include <thread>

#include "encode.hh"
#include <pacerPipe.hh>
#include <quicRClient.hh>
#include <udpPipe.hh>

using namespace MediaNet;

int main(int argc, char *argv[]) {
  std::string relayName("localhost");

  if (argc > 2) {
    std::cerr << "Usage: " << argv[0] << " <hostname>" << std::endl;
    return -1;
  }

  if (argc == 2) {
    relayName = std::string(argv[1]);
  }
  QuicRClient qClient;
  qClient.setCryptoKey(1, sframe::bytes(8, uint8_t(1)));
  qClient.open(1, relayName, 5004, 1);

  while (!qClient.ready()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
  std::cout << "Transport is ready" << std::endl;

  // setup up sending rate and size

  const int packetsPerSecond = 1000;
  const int headerBytes = 65;

  int numToSend = 5 * packetsPerSecond;
  int packetCount = 0;
  auto startTimePoint = std::chrono::steady_clock::now();

  ShortName name;
  name.resourceID = 0xAAAAAAAAAAAAAAAA;
  name.sourceID = 0XFF;
  name.senderID = 0x1234;
  name.fragmentID = 0;
  name.mediaTime = packetCount;

  do {
    const int packetGapUs = 1000000 / packetsPerSecond;
    auto sendTime =
        startTimePoint + std::chrono::microseconds(packetGapUs * packetCount++);
    std::this_thread::sleep_until(sendTime);

    uint64_t bitRate = qClient.getTargetUpstreamBitrate(); // in bps

    const int maxBitRete = 3 * 1000 * 1000;
    if (bitRate > maxBitRete) {
      // limit bitRate
      bitRate = maxBitRete;
    }

    uint64_t bytesPerPacket64 = (bitRate / packetsPerSecond) / 8;
    uint32_t bytesPerPacket = (uint32_t)bytesPerPacket64;

    if (bytesPerPacket < headerBytes + 2) {
      bytesPerPacket = headerBytes + 2;
      std::clog << "Warning bitrate too low for packet rate" << std::endl;
    }
    if (bytesPerPacket > 1200) {
      bytesPerPacket = 1200;
      std::clog << "Warning bitrate too high for packet rate (need "
                << bitRate / (8 * 1200) << " pps)" << std::endl;
    }

    // std::clog << "bytesPerPackt=" << bytesPerPacket << std::endl;

    name.mediaTime = packetCount;
    auto packet = qClient.createPacket(name, 1200);

    assert(bytesPerPacket - headerBytes >= 1);
    packet->resize(bytesPerPacket - headerBytes);

		uint8_t *buffer = &(packet->data());
    *buffer++ = 1;

    packet->setFEC(false);
    packet->setReliable(true);
    packet->setPriority(3);

    qClient.publish(move(packet));

    // empty the receive queue
    do {
      auto ack = qClient.recv();
      if (ack) {
        // std::clog << "got an ack" << std::endl;
      } else {
        break;
      }
    } while (true);

  } while (--numToSend > 0);

  return 0;
}
