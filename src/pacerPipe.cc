
#include <cassert>
#include <thread>

#include "encode.hh"
#include "pacerPipe.hh"
#include "packet.hh"

using namespace MediaNet;

PacerPipe::PacerPipe(PipeInterface *t)
    : PipeInterface(t), rateCtrl(this), shutDown(false), oldPhase(0) {
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

std::unique_ptr<Packet> PacerPipe::recv() {
  std::unique_ptr<Packet> ret = std::unique_ptr<Packet>(nullptr);
  {
    std::lock_guard<std::mutex> lock(recvQMutex);
    if (!recvQ.empty()) {
      ret = move(recvQ.front());
      recvQ.pop();

      // std::clog << "-";
    }
  }

  return ret;
}

bool PacerPipe::send(std::unique_ptr<Packet> p) {
    (void)p;
    assert(0);
  return true;
}

void PacerPipe::runNetSend() {
  while (!shutDown) {

    // If in a new cycle, send a rate message to relay
    uint32_t phase = rateCtrl.getPhase();
    if (oldPhase != phase) {

      oldPhase = phase;
      // starting new phase

      auto packet = std::make_unique<Packet>();
      assert(packet);
      // packet->buffer.reserve(20); // TODO - tune the 20

      packet << PacketTag::headerMagicData;
      // packet << PacketTag::extraMagicVer1;
      // packet << PacketTag::extraMagicVer2;

      NetRateReq rateReq{};
      rateReq.bitrateKbps = toVarInt(rateCtrl.bwDownTarget() / 1000); // TODO
      packet << rateReq;

      // std::clog << "Send Rate Req" << std::endl;
      downStream->send(move(packet));
    }

    std::unique_ptr<Packet> packet = upStream->toDownstream();

    if ( !packet ) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
      continue;
    }

    // TODO - watch bitrate and don't send until OK to send more data

    NetClientSeqNum seqTag{};
    static uint32_t nextSeqNum = 0; // TODO - add mutex etc
    seqTag.clientSeqNum = (nextSeqNum++);
    packet << seqTag;

    std::chrono::steady_clock::time_point tp = std::chrono::steady_clock::now();
    std::chrono::steady_clock::duration dn = tp.time_since_epoch();
    uint32_t nowUs =
        (uint32_t)std::chrono::duration_cast<std::chrono::microseconds>(dn)
            .count();

    uint16_t bits = (uint16_t)packet->fullSize() * 8 +
                    42 * 8; // Capture shows 42 byte header before UDP payload
                            // including ethernet frame

    assert(packet);
    rateCtrl.sendPacket((seqTag.clientSeqNum), nowUs, bits,
                        packet->shortName());

    downStream->send(move(packet));

    // std::clog << ">";
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

    while (nextTag(packet) ==
           PacketTag::ack) { // TODO - move to if and allow only one
      NetAck ackTag{};
      packet >> ackTag;
      rateCtrl.recvAck(ackTag.netAckSeqNum, ackTag.netRecvTimeUs, nowUs);
      nowUs = 0; // trash receive time for lost ACKs (all but first)
    }

    // look for incoming relaySeqNum
    if (nextTag(packet) == PacketTag::relaySeqNum) {
      NetRelaySeqNum relaySeqNum{};
      packet >> relaySeqNum;

      uint16_t bits = (uint16_t)packet->fullSize() * 8 +
                      42 * 8; // Capture shows 42 byte header before UDP payload
                              // including ethernet frame

      rateCtrl.recvPacket(relaySeqNum.relaySeqNum, relaySeqNum.remoteSendTimeMs,
                          nowUs, bits);
      nowUs = 0; // trash receive time for lost ACKs (all but first)
    }

    {
      std::lock_guard<std::mutex> lock(recvQMutex);
      recvQ.push(move(packet));
      // TODO - check Q not too deep
    }

    // std::clog << "<";
  }
}

uint64_t PacerPipe::getTargetUpstreamBitrate() { return rateCtrl.bwUpTarget(); }
