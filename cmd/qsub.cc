#include <chrono>
#include <iostream>
#include <thread>

#include "encode.hh"
#include <pacerPipe.hh>
#include <quicRClient.hh>
#include <udpPipe.hh>
#include <algorithm>
#include <functional>

using namespace MediaNet;

int main(int argc, char *argv[]) {
  std::string relayName("localhost");

  if (argc < 3) {
    std::cerr << "Usage: " << argv[0] << " <hostname> <shortname>" << std::endl;
    std::cerr << "<shortname>: qr://<resourceId>/<senderId>/<sourceId>;" << std::endl;
    std::cerr << "resourceId, senderId, sourceId, mediaTime are integers." << std::endl;
    std::cerr << "senderId, sourceId, mediaTime optional" << std::endl;
    return -1;
  }

  if (argc == 2) {
    relayName = std::string(argv[1]);
  }

  auto shortName = ShortName::fromString(argv[2]);

  std::cout << "Subscribing to ->" << shortName << std::endl;
  QuicRClient qClient;
  qClient.setCryptoKey(1, sframe::bytes(8, uint8_t(1)));
  qClient.open(1, relayName, 5004, 1);

  while (!qClient.ready()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }

  std::cout << "Transport is ready" << std::endl;

	qClient.subscribe(shortName);

  int numRecv = 0;
  // empty the receive queue
  do {
    auto packet = qClient.recv();
    if (!packet) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
      continue;
    }
    // std::clog << "bufSz=" << packet->size() << std::endl;
    numRecv++;
  } while (numRecv < 100 * 1000);

  return 0;
}
