// File: /home/kami/ics-guard/include/ics_guard/protocols/iec104.hpp

#pragma once

#include "ics_guard/core/types.hpp"
#include <cstdint>
#include <span>

namespace ics_guard::protocols {

struct Iec104Policy {
    bool allow_telecontrol_activation{false};
};

class Iec104Validator {
public:
    static constexpr size_t MIN_APCI_LEN = 6;
    static constexpr size_t MIN_ASDU_LEN = 12;

    [[nodiscard]] static GuardStatus validate(
        std::span<const uint8_t> frame,
        const Iec104Policy& policy = {}) noexcept;
};

} // namespace ics_guard::protocols