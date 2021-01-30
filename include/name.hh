#pragma once

#include <cstdint>


namespace MediaNet {


    struct ShortName {
        uint64_t resourceID;
        uint32_t senderID;
        uint8_t sourceID;
        uint32_t mediaTime;
        uint8_t fragmentID;
    };



} // namespace
