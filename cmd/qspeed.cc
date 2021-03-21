

#include <cassert>
#include <chrono>
#include <iostream>
#include <thread>

#include "../src/encode.hh"
#include <quicr/quicRClient.hh>

using namespace MediaNet;

int main(int argc, char *argv[]) {
  std::string relayName("localhost");

  if (argc != 2) {
    std::cerr << "Usage: " << argv[0] << " <hostname>" << std::endl;
    return -1;
  }
  relayName = std::string(argv[1]);

  QuicRClient qClient;
  qClient.setCryptoKey(1, sframe::bytes(8, uint8_t(1)));
  qClient.open(1, relayName, 5004, 1);

  while (!qClient.ready()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
  std::cout << "Transport is ready" << std::endl;

  // setup up sending rate and size

  // const int maxSpeedUpBps = 5 * 1000 * 1000;
  const int timeToSendUpSeconds = 30;
  const int packetsUpPerSecond = 60;

  qClient.setBitrateUp(1e6, 3e6, 4e6);
  qClient.setRttEstimate(50);
  qClient.setPacketsUp(500, 240);

  const int packetSizeByes =
      100; // TODO FIX (maxSpeedUpBps / packetsUpPerSecond) / 8;
  assert(packetSizeByes < 32768);

  int packetCount = 0;
  auto startTimePoint = std::chrono::steady_clock::now();
  uint32_t lastPrintTime = 0;

  ShortName name(1, 1, 1);
  name.mediaTime = packetCount;

  // test Speed Upstream
  do {
    packetCount++;

    const int packetGapUs = 1000000 / packetsUpPerSecond;
    auto sendTime =
        startTimePoint + std::chrono::microseconds(packetGapUs * packetCount);
    std::this_thread::sleep_until(sendTime);

    name.mediaTime = packetCount;
    auto packet = qClient.createPacket(name, packetSizeByes);
    packet->resize(packetSizeByes);

    packet->setFEC(false);
    packet->setReliable(false);
    packet->setPriority(1);

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

    auto now = std::chrono::steady_clock::now();
    uint32_t durationMs =
        (uint32_t)std::chrono::duration_cast<std::chrono::milliseconds>(
            now - startTimePoint)
            .count();
    if (durationMs > timeToSendUpSeconds * 1000) {
      break;
    }

    if (durationMs > lastPrintTime + 500) {
      lastPrintTime = durationMs;

      uint64_t bitRate = qClient.getTargetUpstreamBitrate(); // in bps
      std::clog << "Send Up rate: " << float(bitRate) / 1e6 << " mbps"
                << std::endl;
    }

  } while (true);

  return 0;
}
