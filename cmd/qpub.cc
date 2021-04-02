

#include <cassert>
#include <chrono>
#include <iostream>
#include <thread>

//#include "../src/encode.hh"
//#include "../src/pacerPipe.hh"
//#include "../src/udpPipe.hh"

#include <quicr/quicRClient.hh>

using namespace MediaNet;

int
main(int argc, char* argv[])
{
  std::string relayName("localhost");

  if (argc < 3) {
    std::cerr << "Usage: " << argv[0] << " <hostname> <shortname>" << std::endl;
    std::cerr << "\t<shortname>: qr://<resourceId>/<senderId>/<sourceId>/"
              << std::endl;
    std::cerr << "\t Example: qr://1234/12/1/" << std::endl;
    std::cerr << "resourceId, senderId, sourceId are MANDATORY & integers."
              << std::endl;
    return -1;
  }

  QuicRClient qClient;
  qClient.setCryptoKey(1, sframe::bytes(8, uint8_t(1)));
  qClient.open(1, relayName, 5004, 1);

  while (!qClient.ready()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
  std::cout << "Transport is ready" << std::endl;

  // setup up sending rate and size

  const int packetsPerSecond = 600;
  const int transportHeaderBytes = 65;

  int numToSend = 20 * packetsPerSecond;
  int packetCount = 0;
  auto startTimePoint = std::chrono::steady_clock::now();

  auto name = ShortName::fromString(argv[2]);
  name.fragmentID = 0;
  name.mediaTime = packetCount;

  do {
    const int packetGapUs = 1000000 / packetsPerSecond;
    auto sendTime =
      startTimePoint + std::chrono::microseconds(packetGapUs * packetCount++);
    std::this_thread::sleep_until(sendTime);

    uint64_t bitRate = qClient.getTargetUpstreamBitrate(); // in bps

    const int maxBitRate = 100 * 1000 * 1000;
    if (bitRate > maxBitRate) {
      // limit bitRate
      bitRate = maxBitRate;
    }

    uint64_t bytesPerPacket = (bitRate / packetsPerSecond) / 8;

    bytesPerPacket = 500; // TODO - remove

    if (bytesPerPacket < transportHeaderBytes + 2) {
      bytesPerPacket = transportHeaderBytes + 2;
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

    assert(bytesPerPacket - transportHeaderBytes >= 1);
    packet->resize(bytesPerPacket - transportHeaderBytes);

    uint8_t* buffer = &(packet->data());
    *buffer++ = 1;

    packet->setFEC(false);
    packet->setReliable(false);
    packet->setPriority(3);

    qClient.publish(move(packet));

    // empty the receive queue
    do {
      auto ack = qClient.recv();
      if (ack) {
        std::clog << "got an ack" << std::endl;
      } else {
        break;
      }
    } while (true);

  } while (--numToSend > 0);

  return 0;
}
