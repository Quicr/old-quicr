
#include <cassert>

#include "encode.hh"
#include "quicr/quicRServer.hh"

#include "connectionPipe.hh"
#include "fakeLossPipe.hh"
#include "udpPipe.hh"

using namespace MediaNet;
// TODO: Add other elements for pipeline - loss,spinBit, rateCtrl
QuicRServer::QuicRServer()
  : udpPipe()
  , fakeLossPipe(&udpPipe)
  , connectionPipe(&fakeLossPipe)
  , firstPipe(&connectionPipe)
{
  firstPipe->updateMTU(1280, 500);
}

QuicRServer::~QuicRServer()
{
  firstPipe->stop();
}

bool
QuicRServer::ready() const
{
  return firstPipe->ready();
}

void
QuicRServer::close()
{
  firstPipe->stop();
}

std::unique_ptr<Packet>
QuicRServer::recv()
{

  auto packet = std::unique_ptr<Packet>(nullptr);

  bool bad = true;
  while (bad) {

    packet = firstPipe->recv();

    if (!packet) {
      return packet;
    }

    if (packet->size() < 1) {
      // TODO log bad data
      std::clog << "quicr recv very bad size = " << packet->size() << std::endl;
      continue;
    }

    bad = false;
  }

  // std::clog << "QuicR Server received packet size=" << packet->size() <<
  // std::endl;
  return packet;
}

bool
QuicRServer::open(const uint16_t port)
{
  // TODO: add a start() overload for server flows
  return firstPipe->start(port, "", nullptr);
}

bool
QuicRServer::send(std::unique_ptr<Packet> packet)
{
  // TODO: using UdpPipe directly. Revis this
  return udpPipe.send(std::move(packet));
}
