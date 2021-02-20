
#include <cassert>
#include <thread>

#include "encode.hh"
#include "pacerPipe.hh"
#include "packet.hh"

using namespace MediaNet;

PacerPipe::PacerPipe(PipeInterface *t)
    : PipeInterface(t), rateCtrl(this), shutDown(false), oldPhase(-1),mtu(1200),targetPpsUp(500),
    useConstantPacketRate(true),
      nextSeqNum(1) {
  assert(downStream);
}

PacerPipe::~PacerPipe() {
  shutDown = true; // tell threads to stop

  if (recvThread.joinable()) {
    recvThread.join();
  }
  if (sendThread.joinable()) {
    sendThread.join();
  }
}

bool PacerPipe::start(const uint16_t port, const std::string server,
                      PipeInterface *upStreamLink) {
  // assert( upStreamLink );
  upStream = upStreamLink;

  bool ok = downStream->start(port, server, this);
  if (!ok) {
    return false;
  }

  recvThread = std::thread([this]() { this->runNetRecv(); });
  sendThread = std::thread([this]() { this->runNetSend(); });

  return true;
}

void PacerPipe::stop() {
  assert(downStream);
  shutDown = true;
  downStream->stop();
}

bool PacerPipe::ready() const {
  if (shutDown) {
    return false;
  }
  assert(downStream);
  return downStream->ready();
}



bool PacerPipe::send(std::unique_ptr<Packet> p) {
    (void)p;
    assert(0);
  return true;
}

void PacerPipe::sendRateCommand()
{
  auto packet = std::make_unique<Packet>();
  assert(packet);

  packet << PacketTag::headerMagicData;
  // packet << PacketTag::extraMagicVer1;
  // packet << PacketTag::extraMagicVer2;

  NetRateReq rateReq{};
  rateReq.bitrateKbps = toVarInt(rateCtrl.bwDownTarget() / 1000);
  packet << rateReq;

  // std::clog << "Send Rate Req" << std::endl;
  downStream->send(move(packet));
}

void PacerPipe::runNetSend() {
  while (!shutDown) {

    // If in a new cycle, send a rate message to relay
    uint32_t phase = rateCtrl.getPhase();
    if (oldPhase != phase) {
      // starting new phase
      oldPhase = phase;
      phaseStartTime = std::chrono::steady_clock::now();
      packetsSentThisPhase = 0;

      sendRateCommand();
    }

    std::unique_ptr<Packet> packet = upStream->toDownstream();

    if (!packet) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
      continue;
    }

    NetClientSeqNum seqTag{};
    seqTag.clientSeqNum = nextSeqNum++;
    packet << seqTag;

    std::chrono::steady_clock::time_point tp = std::chrono::steady_clock::now();
    std::chrono::steady_clock::duration dn = tp.time_since_epoch();
    uint32_t nowUs =
        (uint32_t) std::chrono::duration_cast<std::chrono::microseconds>(dn)
            .count();

    uint16_t bits = (uint16_t) packet->fullSize() * 8 +
                    42 * 8; // Capture shows 42 byte header before UDP payload
    // including ethernet frame

    assert(packet);
    rateCtrl.sendPacket((seqTag.clientSeqNum), nowUs, bits,
                        packet->shortName());

    downStream->send(move(packet));
    // std::clog << ">";
    packetsSentThisPhase++;

    if (useConstantPacketRate) {
      // wait until time to send next packet
      uint64_t delayTimeUs = ( uint64_t(packetsSentThisPhase) * 1000000l) / uint64_t(targetPpsUp);
      //std::clog<<"packetsSentThisPhase="<<packetsSentThisPhase << " delayMs="<< delayTimeUs/1000 <<std::endl;

      std::this_thread::sleep_until(phaseStartTime + std::chrono::microseconds(delayTimeUs));
    }

    {
      // watch bitrate and don't send until OK to send more data
      uint64_t targetBitrate = rateCtrl.bwUpTarget();
      uint64_t delayTimeUs = (bits * 1000000l) / targetBitrate;
      std::this_thread::sleep_until(tp + std::chrono::microseconds(delayTimeUs));
    }

  }
}

void PacerPipe::runNetRecv() {
  while (!shutDown) {
    std::unique_ptr<Packet> packet = downStream->recv();
    if (!packet) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
      continue;
    }

    std::chrono::steady_clock::time_point tp = std::chrono::steady_clock::now();
    std::chrono::steady_clock::duration dn = tp.time_since_epoch();
    uint32_t nowUs =
        (uint32_t)std::chrono::duration_cast<std::chrono::microseconds>(dn)
            .count();

    // look for ACKs

    // TODO - move to if and allow only one ???
    bool haveAck=true;
    while (nextTag(packet) == PacketTag::ack) {
      NetAck ackTag{};
      packet >> ackTag;
      bool congested = false; // TODO - add to ACK
      rateCtrl.recvAck(ackTag.netAckSeqNum, ackTag.netRecvTimeUs, nowUs, congested, haveAck );
      haveAck = false; // treat redundant ACK as received but not acks
    }

    // look for incoming remoteSeqNum
    if (nextTag(packet) == PacketTag::relaySeqNum) {
      NetRelaySeqNum relaySeqNum{};
      packet >> relaySeqNum;

      uint16_t bits = (uint16_t)packet->fullSize() * 8 +
                      42 * 8; // Capture shows 42 byte header before UDP payload
                              // including ethernet frame

                              bool congested = false; // TODO - add
      rateCtrl.recvPacket(relaySeqNum.relaySeqNum, relaySeqNum.remoteSendTimeUs,
                          nowUs, bits, congested);
    }

    upStream->fromDownstream( move(packet) );

    // std::clog << "<";
  }
}

uint64_t PacerPipe::getTargetUpstreamBitrate() { return rateCtrl.bwUpTarget(); }


std::unique_ptr<Packet> PacerPipe::recv() {
    // this should never be called
    assert(0);
    return std::unique_ptr<Packet>(nullptr);
}

void PacerPipe::updateMTU(uint16_t val,uint32_t pps) {
    mtu = val;
    targetPpsUp = pps;

    useConstantPacketRate = bool( targetPpsUp > 0 );

    rateCtrl.overrideMtu(mtu,pps);

    PipeInterface::updateMTU(val,pps);
}

void PacerPipe::updateBitrateUp(uint64_t minBps, uint64_t startBps, uint64_t maxBps) {

  rateCtrl.overrideBitrateUp(minBps,startBps,maxBps);

  PipeInterface::updateBitrateUp(minBps, startBps, maxBps);
}

void PacerPipe::updateRTT(uint16_t minRttMs, uint16_t bigRttMs) {
   rateCtrl.overrideRTT(minRttMs,bigRttMs);

  PipeInterface::updateRTT(minRttMs, bigRttMs);
}
