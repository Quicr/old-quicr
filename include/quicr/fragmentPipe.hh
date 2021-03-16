#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <mutex>

#include "packet.hh"
#include "pipeInterface.hh"

namespace MediaNet {

class FragmentPipe : public PipeInterface {
public:
  explicit FragmentPipe(PipeInterface *t);

  bool send(std::unique_ptr<Packet> packet) override;

  void updateStat(StatName stat, uint64_t value) override;

  /// non blocking, return nullptr if no buffer
  std::unique_ptr<Packet> recv() override;

  void updateMTU(uint16_t mtu, uint32_t pps) override;

private:
  uint16_t mtu;

  std::mutex fragListMutex;
  std::map<MediaNet::ShortName, std::unique_ptr<Packet>> fragList;
};

} // namespace MediaNet
