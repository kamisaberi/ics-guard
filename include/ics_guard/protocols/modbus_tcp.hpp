// File: /home/kami/ics-guard/include/ics_guard/protocols/modbus_tcp.hpp

#pragma once

#include "ics_guard/core/types.hpp"
#include <cstdint>
#include <span>

namespace ics_guard::protocols {

struct ModbusPolicy {
    bool allow_writes{false};
    uint16_t protected_reg_start{40000};
    uint16_t protected_reg_end{40100};
};

class ModbusTcpValidator {
public:
    static constexpr size_t MBAP_HEADER_LEN = 8; // 7-byte MBAP + 1-byte Function Code

    [[nodiscard]] static GuardStatus validate(
        std::span<const uint8_t> frame,
        const ModbusPolicy& policy = {}) noexcept;
};

} // namespace ics_guard::protocols