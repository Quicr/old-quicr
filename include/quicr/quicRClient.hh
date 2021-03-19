#pragma once

#include <cstdint>
#include <memory>
#include <string>
//#include <utility> // for pair

#include "connectionPipe.hh"
#include "crazyBitPipe.hh"
#include "encryptPipe.hh"
#include "fakeLossPipe.hh"
#include "fecPipe.hh"
#include "fragmentPipe.hh"
#include "pacerPipe.hh"
#include "packet.hh"
#include "pipeInterface.hh"
#include "priorityPipe.hh"
#include "retransmitPipe.hh"
#include "statsPipe.hh"
#include "subscribePipe.hh"
#include "udpPipe.hh"

namespace MediaNet {

// class UdpPipe;
// class PacerPipe;

class QuicRClient {
public:
  QuicRClient();
  virtual ~QuicRClient();
  virtual bool open(uint32_t clientID, std::string relayName, uint16_t port,
                    uint64_t token);
  virtual bool ready() const;
  virtual void close();
	virtual void setCurrentTime(const std::chrono::time_point<std::chrono::steady_clock>& now);

	// Initialize sframe context with the base secret provided by MLS key
  // exchange Note: This is hard coded secret until we bring in MLS
  void setCryptoKey(sframe::MLSContext::EpochID epoch,
                    const sframe::bytes &mls_epoch_secret);

  void setBitrateUp(uint64_t minBps, uint64_t startBps, uint64_t maxBps);
  void setRttEstimate(uint32_t minRttMs, uint32_t bigRttMs = 0);
  void setPacketsUp(uint16_t pps, uint16_t mtu = 1280);

  /*
* void setEncryptionKey(std::vector<uint8_t> salt, std::vector<uint8_t> key,
                  int authTagLen);
void setDecryptionKey(uint32_t clientID, std::vector<uint8_t> salt,
                  std::vector<uint8_t> key);
*/

  virtual std::unique_ptr<Packet> createPacket(const ShortName &name,
                                               int reservedPayloadSize);
  virtual bool publish(std::unique_ptr<Packet>);

  bool subscribe(ShortName);

  /// non blocking, return nullptr if no buffer
  virtual std::unique_ptr<Packet> recv();

  uint64_t getTargetUpstreamBitrate(); // in bps
  // uint64_t getTargetDownstreamBitrate(); // in bps
  // uint64_t getMaxBandwidth();

  // void subscribe( const std::string origin,
  //               const std::string resource,
  //               uint32_t senderID=0, uint8_t sourceID=0 );

private:

	// timer thread
	// TODO: if app can provide its own timepoint, this
	// thread shouldn't be run
	void runTimerThread();
	std::thread timerThread;


	UdpPipe udpPipe;
  FakeLossPipe fakeLossPipe;
  CrazyBitPipe crazyBitPipe;
  ClientConnectionPipe connectionPipe;
  PacerPipe pacerPipe;
  PriorityPipe priorityPipe;
  RetransmitPipe retransmitPipe;
  FecPipe fecPipe;
  SubscribePipe subscribePipe;
  FragmentPipe fragmentPipe;
  EncryptPipe encryptPipe;
  StatsPipe statsPipe;
  PipeInterface *firstPipe;

  // uint32_t pubClientID;
  // uint64_t secToken;
  bool shutDown = false;
};

} // namespace MediaNet