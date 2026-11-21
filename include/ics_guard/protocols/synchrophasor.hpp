// File: /home/kami/ics-guard/include/ics_guard/protocols/synchrophasor.hpp

#pragma once

#include "ics_guard/core/types.hpp"
#include <span>

namespace ics_guard::protocols {

struct SynchrophasorLimits {
    double min_freq_hz{49.50};
    double max_freq_hz{50.50};
    double max_rocof_hz_s{1.00};
    double max_thd_pct{8.00};
};

class SynchrophasorValidator {
public:
    [[nodiscard]] static GuardStatus validate(
        const SynchrophasorSample& sample,
        const SynchrophasorLimits& limits = {}) noexcept;

    [[nodiscard]] static GuardStatus validate_frame(
        std::span<const uint8_t> frame,
        const SynchrophasorLimits& limits = {}) noexcept;
};

} // namespace ics_guard::protocols