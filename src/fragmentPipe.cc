

#include <cassert>
#include <iostream>

#include "encode.hh"
#include "fragmentPipe.hh"
#include "packet.hh"

using namespace MediaNet;

FragmentPipe::FragmentPipe(PipeInterface *t) : PipeInterface(t),mtu(1200) {}


void FragmentPipe::updateStat(PipeInterface::StatName stat, uint64_t value) {
    if ( stat == PipeInterface::StatName::mtu ) {
        mtu = (int)value;
    }

    PipeInterface::updateStat(stat, value);
}


bool FragmentPipe::send(std::unique_ptr<Packet> packet) {
    assert(downStream);

    //std::clog << "Frag::Send packet " << *packet << std::endl;

    // TODO  break packets larger than mtu bytes into equal size fragments less
    mtu = 128; // TODO REMOVE - just for testing
    int extraHeaderSize = 25; // TODO tune

    if ( packet->fullSize()  + extraHeaderSize <= mtu  ) {
        return downStream->send(move(packet));
    }

    assert( nextTag(packet) == PacketTag::appData );
    bool ok = true;

    uint16_t payloadSize;
    PacketTag  tag;

    packet >> tag;
    packet >> payloadSize;
    uint16_t dataSize = mtu - ( extraHeaderSize +  ( packet->fullSize() - packet->size()  ) );
    if ( dataSize < 56 ) { dataSize = 56; }
    assert( dataSize > 1 );
    assert( payloadSize == packet->size() );

    size_t   numDone = 0;
    size_t   numLeft = packet->size();
    uint8_t  frag = 1;

    while (numLeft > 0 ) {
        size_t numUse = dataSize;
        if ( numUse > numLeft ) {
            numUse = numLeft;
        }

        std::unique_ptr<Packet> fragPacket = packet->clone(); // TODO - make a clone just base

        fragPacket->resize( numUse );
        std::copy( &(packet->data())+numDone, &(packet->data())+numDone+numUse, &(fragPacket->data()) );
        numDone += numUse;
        numLeft -= numUse;

        fragPacket << (uint16_t )numUse;
        fragPacket << PacketTag::appDataFrag;

        fragPacket->setFragID( frag*2 + ((numLeft>0)?0:1) );

        if ( numLeft <= 0 ) {
            // TODO - some way to indicate last fragment
        }

        //std::clog << "Send Frag: " << *fragPacket << std::endl;

        //std::clog << "Frag Send:" << fragPacket->shortName() << std::endl;

        ok &= downStream->send(move(fragPacket));

        frag++;
    }

    return ok;
}

std::unique_ptr<Packet> FragmentPipe::recv() {
    // TODO - cache fragments and reassemble then pass on the when full packet is received

  assert(downStream);
  auto packet =  downStream->recv();

  if ( packet ) {
      if ( nextTag(packet) == PacketTag::appDataFrag ) {
          //std::clog << "Frag recv:" << *packet << std::endl;
          std::clog << "Frag Recv:" << packet->shortName()  << std::endl;
      }
  }

  return packet;
}
