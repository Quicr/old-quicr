

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
  qClient.open(1, relayName, 5004, 1);

  while (!qClient.ready()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
  std::cout << "Transport is ready" << std::endl;

  int numRecv = 0;

  // empty the receive queue
  do {
    auto packet = qClient.recv();
    if (!packet) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      continue;
    }

    // std::clog << "bufSz=" << packet->buffer.size() << std::endl;
    numRecv++;
  } while (numRecv < 100000);

  return 0;
}
