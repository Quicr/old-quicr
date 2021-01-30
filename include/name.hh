#pragma once

#include <cstdint>
#include <iostream>

namespace MediaNet {


    struct ShortName {
        uint64_t resourceID;
        uint32_t senderID;
        uint8_t sourceID;
        uint32_t mediaTime;
        uint8_t fragmentID;
    };

    std::ostream& operator<<(std::ostream& stream, const ShortName& name);


} // namespace