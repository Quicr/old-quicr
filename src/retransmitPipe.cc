

#include <cassert>
#include <iostream>

#include "encode.hh"
#include "packet.hh"
#include "retransmitPipe.hh"

using namespace MediaNet;

RetransmitPipe::RetransmitPipe(PipeInterface *t) : PipeInterface(t) {}

bool RetransmitPipe::send(std::unique_ptr<Packet> packet) {
  assert(downStream);

  // TODO - for reliably packets, cache them and resend it no ack received for
  // them

  return downStream->send(move(packet));
}

std::unique_ptr<Packet> RetransmitPipe::recv() {
  assert(downStream);

  // TODO - watch for ACK to stop retranmitions

  return downStream->recv();
}
