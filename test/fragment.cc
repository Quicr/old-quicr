
#include "../src/encode.hh"
#include "../src/fragmentPipe.hh"
#include "quicr/packet.hh"
#include "quicr/quicRClient.hh"
#include "quicr/shortName.hh"
#include <doctest/doctest.h>
#include <memory>
#include <queue>

using namespace MediaNet;

class FakePipe : public PipeInterface
{
public:
  explicit FakePipe(PipeInterface* t)
    : PipeInterface(t)
  {}

  bool send(std::unique_ptr<Packet> packet) override
  {
    upstreamCount++;
    if (nextTag(packet) == PacketTag::clientData) {
      ClientData client_data;
      packet >> client_data;
    }
    fragmented_packets.push_back(std::move(packet));
    return true;
  }

  ShortName name;
  std::deque<std::unique_ptr<Packet>> fragmented_packets;
  int upstreamCount = 0;
};

static std::unique_ptr<Packet>
generate_large_packet(int size)
{

  // roughly 10-12 fragments [ 1280 - extraHeaderBytes - header - metadata ]
  QuicRClient client;
  auto packet =
    client.createPacket(ShortName::fromString("qr://1234/12/"), size);
  assert(packet);
  std::vector<uint8_t> image_data;
  image_data.resize(size);
  // add the payload
  packet->push_back(image_data);

  // add the header tags
  ClientData clientData;
  clientData.clientSeqNum = 0;
  NamedDataChunk namedDataChunk;
  namedDataChunk.shortName = packet->shortName();
  namedDataChunk.lifetime = toVarInt(0); // TODO

  DataBlock dataBlock;
  dataBlock.metaDataLen = toVarInt(0);
  dataBlock.dataLen = toVarInt(packet->size());

  packet << dataBlock;
  packet << namedDataChunk;
  packet << clientData;

  return packet;
}

TEST_CASE("verify fragmentation and re-assembly")
{
  auto fake_pipe = new FakePipe(nullptr);
  constexpr int payload_size = 12000;
  auto fragment_pipe = std::make_unique<FragmentPipe>(fake_pipe);
  auto large_packet_orig = generate_large_packet(payload_size);
  auto large_packet_transmitted = generate_large_packet(payload_size);
  fragment_pipe->send(std::move(large_packet_transmitted));
  REQUIRE_GE(fake_pipe->upstreamCount, 10);
  // assembly
  while (!fake_pipe->fragmented_packets.empty()) {
    auto& packet = fake_pipe->fragmented_packets.front();
    auto result = fragment_pipe->processRxPacket(std::move(packet));
    fake_pipe->fragmented_packets.pop_front();
    if (result) {
      REQUIRE_EQ(result->shortName().fragmentID, 1);
      NamedDataChunk namedDataChunk;
      DataBlock block;
      result >> namedDataChunk;
      result >> block;
      REQUIRE_EQ(fromVarInt(block.dataLen), payload_size);
    }
  }
}
