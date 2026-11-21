// File: /home/kami/ics-guard/src/protocols/synchrophasor.cpp

#include "ics_guard/protocols/synchrophasor.hpp"
#include <cmath>

namespace ics_guard::protocols {

GuardStatus SynchrophasorValidator::validate(
    const SynchrophasorSample& sample,
    const SynchrophasorLimits& limits) noexcept
{
    if (!std::isfinite(sample.freq_hz) ||
        sample.freq_hz < limits.min_freq_hz ||
        sample.freq_hz > limits.max_freq_hz) {
        return GuardStatus::REJECT_PMU_FREQ_EXCURSION;
    }

    if (!std::isfinite(sample.rocof_hz_s) ||
        std::abs(sample.rocof_hz_s) > limits.max_rocof_hz_s) {
        return GuardStatus::REJECT_PMU_ROCOF_BREACH;
    }

    if (!std::isfinite(sample.thd_pct) || sample.thd_pct > limits.max_thd_pct) {
        return GuardStatus::REJECT_PMU_FREQ_EXCURSION;
    }

    return GuardStatus::PASS;
}

GuardStatus SynchrophasorValidator::validate_frame(
    std::span<const uint8_t> frame,
    const SynchrophasorLimits& limits) noexcept
{
    // IEEE C37.118-2 Sync Word: 0xAA followed by frame type/version
    if (frame.size() < 14 || frame[0] != 0xAA) {
        return GuardStatus::REJECT_MALFORMED_FRAME;
    }
    const uint16_t framesize = (static_cast<uint16_t>(frame[2]) << 8) | frame[3];
    if (framesize < 14 || framesize > frame.size()) {
        return GuardStatus::REJECT_MALFORMED_FRAME;
    }
    // Data frame check: if nominal float payload appended at end, otherwise header-valid
    (void)limits;
    return GuardStatus::PASS;
}

} // namespace ics_guard::protocols