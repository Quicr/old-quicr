

#include <algorithm>
#include <cassert>
#include <iostream>

#include "encode.hh"
#include "fragmentPipe.hh"
#include "packet.hh"

using namespace MediaNet;

FragmentPipe::FragmentPipe(PipeInterface *t) : PipeInterface(t), mtu(1200) {}

void FragmentPipe::updateStat(PipeInterface::StatName stat, uint64_t value) {
  if (stat == PipeInterface::StatName::mtu) {
    mtu = (int)value;
  }

  PipeInterface::updateStat(stat, value);
}

bool FragmentPipe::send(std::unique_ptr<Packet> packet) {
  assert(downStream);

  // std::clog << "Frag::Send packet " << *packet << std::endl;

  // TODO  break packets larger than mtu bytes into equal size fragments less

  const int extraHeaderSizeBytes = 25; // TODO tune and move
  const int minPacketPayload = 56;     // TODO move

  if (packet->fullSize() + extraHeaderSizeBytes <= mtu) {
    // std::clog << "no fragment as size=" << packet->fullSize() << " mtu=" <<
    // mtu << std::endl;
    return downStream->send(move(packet));
  }

  assert(0); // this is broken with new encndoing

  assert(nextTag(packet) == PacketTag::pubData);
  bool ok = true;

  PacketTag tag;
  ShortName name;
  uintVar_t lifetime;
  uint8_t authTagLen;
  uintVar_t payloadSize;

  packet >> tag;
  packet >> name;
  packet >> lifetime;
  packet >> authTagLen;
  packet >> payloadSize;

  // std::clog << "fragment input size: " << payloadSize << std::endl;

  uint16_t dataSize =
      mtu - (extraHeaderSizeBytes + (packet->fullSize() - packet->size()));
  if (dataSize < minPacketPayload) {
    dataSize = minPacketPayload;
  }
  assert(dataSize > 1);
  // assert(payloadSize == packet->size()); TODO remove

  size_t numDone = 0;
  size_t numLeft = packet->size();
  uint8_t frag = 1;

  while (numLeft > 0) {
    size_t numUse = std::min(size_t(dataSize), numLeft);

    std::unique_ptr<Packet> fragPacket = packet->clone();

    fragPacket->resize(numUse);
    std::copy(&(packet->data()) + numDone, &(packet->data()) + numDone + numUse,
              &(fragPacket->data()));
    numDone += numUse;
    numLeft -= numUse;

    fragPacket->setFragID(frag, (numLeft == 0));

    fragPacket << (uint16_t)numUse;

    fragPacket << packet->shortName();
    fragPacket << PacketTag::pubData;

    // std::clog << "Send Frag: " << *fragPacket << std::endl;
    // std::clog << "Frag Send:" << fragPacket->shortName() << std::endl;

    ok &= downStream->send(move(fragPacket));

    frag++;

    assert(frag < 64);
  }

  return ok;
}

std::unique_ptr<Packet> FragmentPipe::recv() {
  // TODO - cache fragments and reassemble then pass on the when full packet is
  // received
  assert(downStream);

  while (true) {

    auto packet = downStream->recv();
    if (!packet) {
      return packet;
    }

    // TODO - this is broken now, need to looka at shortname to decide if this
    // is a fragment or not

    if (nextTag(packet) != PacketTag::subData) {
      return packet;
    }

    while (nextTag(packet) != PacketTag::subData) {

      auto packetCopy = packet->clone();

      uint16_t payloadSize;
      PacketTag tag;
      ShortName name;

      packet >> tag;
      packet >> name;
      packet >> payloadSize;
      assert(packet->size() >= payloadSize);
      packet->resize(packet->size() - payloadSize);

      packetCopy->name = name;

      // std::clog << "Frag Recv:" << name << " size=" << packet->size() <<
      // *packet << std::endl;

      auto p = std::pair<MediaNet::ShortName, std::unique_ptr<Packet>>(
          name, move(packetCopy));
      {
        std::lock_guard<std::mutex> lock(fragListMutex);
        fragList.insert(move(p));
      }

      // TODO - clear out old fragments

      // check if we have all the fragments
      bool haveAll = true;
      int frag = 0;
      int numFrag = 0;
      while (haveAll) {
        frag++;
        ShortName fragName = name;
        fragName.fragmentID = frag * 2;
        if (fragList.find(fragName) != fragList.end()) {
          // exists but is not the last fragment
          continue;
        }
        fragName.fragmentID = frag * 2 + 1;
        if (fragList.find(fragName) != fragList.end()) {
          // exists and is the last element
          numFrag = frag;
          break;
        }
        haveAll = false;
      }

      if (haveAll) {
        // form the new packet from all fragments and return
        // std::clog << "HAVE ALL for: " << name << std::endl;
        ShortName fragName = name;

        auto result = std::unique_ptr<Packet>(nullptr);

        for (int i = 1; i <= numFrag; i++) {

          fragName.fragmentID = i * 2 + ((i == numFrag) ? 1 : 0);
          auto fragPair = fragList.find(fragName);
          assert(fragPair != fragList.end());
          std::unique_ptr<Packet> fragPacket = move(fragPair->second);
          assert(fragPacket);
          fragList.erase(fragPair);

          uint16_t dataSize;
          ShortName fragName;
          PacketTag tag;

          // std::clog << "Adding fragment Name: " << fragPacket->shortName() <<
          // std::endl;

          fragPacket >> tag;
          fragPacket >> fragName;
          fragPacket >> dataSize;

          assert(fragPacket);
          assert(fragPacket->size() > 0);
          assert(tag == PacketTag::subData);
          assert(dataSize > 0);
          assert(dataSize <= fragPacket->size());

          if (i == 1) {
            // use the first fragment as the result packet
            result = move(fragPacket);
            result->setFragID(0, true);
            assert(dataSize <= result->fullSize());
            result->headerSize = (int)(result->fullSize()) - dataSize;
            continue;
          }

          assert(result);
          assert(fragPacket);
          result->resize((int)(result->size()) + dataSize);

          uint8_t *src = &(fragPacket->data()) + fragPacket->size() - dataSize;
          uint8_t *end = &(fragPacket->data()) + fragPacket->size();
          uint8_t *dst = &(result->data()) + result->size() - dataSize;

          std::copy(src, end, dst);
        }

        assert(result);
        name.fragmentID = 0;
        uint16_t payloadSize = result->size();
        result << payloadSize;

        result << name;
        result << PacketTag::pubData;

        result->name = name;
        result->setFragID(0, true);

        return result;
      }
    }
  }
}

void FragmentPipe::updateMTU(uint16_t val, uint32_t pps) {
  mtu = val;

  PipeInterface::updateMTU(val, pps);
}

// TODO - need unit test for de-fragment
