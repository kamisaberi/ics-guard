// File: /home/kami/ics-guard/include/ics_guard/core/types.hpp

#pragma once

#include <cstdint>
#include <string_view>

namespace ics_guard {

enum class GuardStatus : uint8_t {
    PASS = 0,
    REJECT_MALFORMED_FRAME,
    REJECT_MODBUS_ILLEGAL_FC,
    REJECT_MODBUS_UNAUTHORIZED_WRITE,
    REJECT_S7_CPU_STOP,
    REJECT_S7_UNAUTHORIZED_WRITE,
    REJECT_IEC104_SPURIOUS_TRIP,
    REJECT_PMU_FREQ_EXCURSION,
    REJECT_PMU_ROCOF_BREACH,
    REJECT_CBF_INVARIANCE_BREACH,
    ACTUATOR_AIRGAP_TRIPPED
};

[[nodiscard]] constexpr std::string_view status_to_string(GuardStatus st) noexcept {
    switch (st) {
        case GuardStatus::PASS:                             return "PASS";
        case GuardStatus::REJECT_MALFORMED_FRAME:           return "REJECT_MALFORMED_FRAME";
        case GuardStatus::REJECT_MODBUS_ILLEGAL_FC:         return "REJECT_MODBUS_ILLEGAL_FC";
        case GuardStatus::REJECT_MODBUS_UNAUTHORIZED_WRITE: return "REJECT_MODBUS_UNAUTHORIZED_WRITE";
        case GuardStatus::REJECT_S7_CPU_STOP:               return "REJECT_S7_CPU_STOP";
        case GuardStatus::REJECT_S7_UNAUTHORIZED_WRITE:     return "REJECT_S7_UNAUTHORIZED_WRITE";
        case GuardStatus::REJECT_IEC104_SPURIOUS_TRIP:      return "REJECT_IEC104_SPURIOUS_TRIP";
        case GuardStatus::REJECT_PMU_FREQ_EXCURSION:        return "REJECT_PMU_FREQ_EXCURSION";
        case GuardStatus::REJECT_PMU_ROCOF_BREACH:          return "REJECT_PMU_ROCOF_BREACH";
        case GuardStatus::REJECT_CBF_INVARIANCE_BREACH:     return "REJECT_CBF_INVARIANCE_BREACH";
        case GuardStatus::ACTUATOR_AIRGAP_TRIPPED:          return "ACTUATOR_AIRGAP_TRIPPED";
    }
    return "UNKNOWN";
}

struct SynchrophasorSample {
    double freq_hz{50.0};
    double rocof_hz_s{0.0};
    double thd_pct{1.4};
    double busbar_kv{220.4};
};

} // namespace ics_guard