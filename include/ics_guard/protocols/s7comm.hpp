// File: /home/kami/ics-guard/include/ics_guard/protocols/s7comm.hpp

#pragma once

#include "ics_guard/core/types.hpp"
#include <cstdint>
#include <span>

namespace ics_guard::protocols {

struct S7Policy {
    bool allow_db_writes{false};
    bool block_plc_control{true};
};

class S7CommValidator {
public:
    static constexpr size_t MIN_TPKT_COTP_LEN = 7;
    static constexpr size_t MIN_S7_PDU_LEN = 18;

    [[nodiscard]] static GuardStatus validate(
        std::span<const uint8_t> frame,
        const S7Policy& policy = {}) noexcept;
};

} // namespace ics_guard::protocols