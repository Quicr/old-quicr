

#include <algorithm>
#include <cassert>
#include <iostream>

#include "encode.hh"
#include "fragmentPipe.hh"
#include "quicr/packet.hh"

using namespace MediaNet;

FragmentPipe::FragmentPipe(PipeInterface* t)
  : PipeInterface(t)
  , mtu(1200)
{}

void
FragmentPipe::updateStat(PipeInterface::StatName stat, uint64_t value)
{
  if (stat == PipeInterface::StatName::mtu) {
    mtu = (int)value;
  }

  PipeInterface::updateStat(stat, value);
}

bool
FragmentPipe::send(std::unique_ptr<Packet> packet)
{
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

  ClientData clientData;
  NamedDataChunk namedDataChunk;
  EncryptedDataBlock encryptedDataBlock;

  bool ok = true;
  assert(nextTag(packet) == PacketTag::clientData);
  ok &= packet >> clientData;
  ok &= packet >> namedDataChunk;
  assert(ok);
  assert(nextTag(packet) == PacketTag::encDataBlock);
  ok &= packet >> encryptedDataBlock;
  assert(ok);

  assert(namedDataChunk.shortName.fragmentID == 0);
  assert(fromVarInt(encryptedDataBlock.metaDataLen) == 0); // TODO

  uint16_t dataSize = mtu - extraHeaderSizeBytes;
  if (dataSize < minPacketPayload) {
    dataSize = minPacketPayload;
  }
  assert(dataSize > 1);

  size_t numDone = 0;
  size_t numLeft = packet->size();
  uint8_t frag = 1;

  while (numLeft > 0) {
    size_t numUse = std::min(size_t(dataSize), numLeft);
    std::unique_ptr<Packet> fragPacket = packet->clone();

    fragPacket->resize(numUse);
    std::copy(&(packet->data()) + numDone,
              &(packet->data()) + numDone + numUse,
              &(fragPacket->data()));
    numDone += numUse;
    numLeft -= numUse;

    fragPacket->setFragID(frag, (numLeft == 0));
    namedDataChunk.shortName = packet->shortName();
    encryptedDataBlock.cipherDataLen = toVarInt(numUse);

    fragPacket << encryptedDataBlock;
    fragPacket << namedDataChunk;
    fragPacket << clientData;

    // std::clog << "Send Frag: " << *fragPacket << std::endl;
    // std::clog << frag << ": Frag Send:" << fragPacket->shortName() <<
    // std::endl;

    ok &= downStream->send(move(fragPacket));

    frag++;

    assert(frag < 64);
  }

  return ok;
}

std::unique_ptr<Packet>
FragmentPipe::recv()
{
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

    if (nextTag(packet) != PacketTag::shortName) {
      return packet;
    }

    while (nextTag(packet) == PacketTag::shortName) {
      NamedDataChunk namedDataChunk;
      EncryptedDataBlock encryptedDataBlock;
      bool ok = true;
      ok &= packet >> namedDataChunk;
      ok &= packet >> encryptedDataBlock;
      if (!ok) {
        //  TODO log bad packet
        return std::unique_ptr<Packet>(nullptr);
      }
      if (namedDataChunk.shortName.fragmentID == 0) {
        // packet wasn't fragmented
        // TODO (1): add explicit marking instead of checking fragmentID?
        // TODO (2): hide the pop and push back tag semantics behind an api
        // std::clog << "frag recv: unfragmented:" << namedDataChunk.shortName
        // << std::endl;
        packet << encryptedDataBlock;
        packet << namedDataChunk;
        return packet;
      }

      uint16_t payloadSize = fromVarInt(encryptedDataBlock.cipherDataLen);
      assert(packet->size() >= payloadSize); // TODO - change log error
      packet->resize(packet->size() - payloadSize);

      auto packetCopy = packet->clone();
      packetCopy->name = namedDataChunk.shortName;
      // TODO - set lifetime in packetCopy

      // std::clog << "Frag Recv:" << namedDataChunk.shortName << " size=" <<
      // packet->size() << std::endl; *packet << std::endl;

      {
        std::lock_guard<std::mutex> lock(fragListMutex);
        // std::clog << "fragList: added:" << namedDataChunk.shortName <<
        // std::endl;
        fragList.emplace(namedDataChunk.shortName, move(packetCopy));
      }

      // TODO - clear out old fragments

      // check if we have all the fragments
      bool haveAll = true;
      int frag = 0;
      int numFrag = 0;
      while (haveAll) {
        frag++;
        std::clog << "processing frag: " << frag << std::endl;
        ShortName fragName = namedDataChunk.shortName;
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
        // std::clog << "HAVE ALL for: " << namedDataChunk.shortName <<
        // std::endl;
        ShortName fragName = namedDataChunk.shortName;

        auto result = std::unique_ptr<Packet>(nullptr);

        for (int i = 1; i <= numFrag; i++) {
          fragName.fragmentID = i * 2 + ((i == numFrag) ? 1 : 0);
          auto fragPair = fragList.find(fragName);
          assert(fragPair != fragList.end());
          std::unique_ptr<Packet> fragPacket = move(fragPair->second);
          assert(fragPacket);
          fragList.erase(fragPair);

          NamedDataChunk namedDataChunk;
          EncryptedDataBlock encryptedDataBlock;

          fragPacket >> namedDataChunk;
          fragPacket >> encryptedDataBlock;

          // std::clog << "(HAVEALL)Adding fragment Name: " <<
          // namedDataChunk.shortName << std::endl;

          uint32_t dataSize = fromVarInt(encryptedDataBlock.cipherDataLen);

          assert(fragPacket->size() > 0);
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

          uint8_t* src = &(fragPacket->data()) + fragPacket->size() - dataSize;
          uint8_t* end = &(fragPacket->data()) + fragPacket->size();
          uint8_t* dst = &(result->data()) + result->size() - dataSize;

          std::copy(src, end, dst);
        }

        assert(result);
        namedDataChunk.shortName.fragmentID = 0;

        encryptedDataBlock.cipherDataLen = toVarInt(result->size());

        result << encryptedDataBlock;
        result << namedDataChunk;

        result->name = namedDataChunk.shortName;
        result->setFragID(0, true);
        return result;
      }
    }
  }
}

void
FragmentPipe::updateMTU(uint16_t val, uint32_t pps)
{
  mtu = val;

  PipeInterface::updateMTU(val, pps);
}

// TODO - need unit test for de-fragment
