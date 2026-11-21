// File: /home/kami/ics-guard/include/ics_guard/defense/cbf_safety_gate.hpp

#pragma once

#include "ics_guard/core/types.hpp"
#include "ics_guard/defense/plant_ode.h"

namespace ics_guard::defense {

struct CbfSafetyLimits {
    double max_temp_c{120.0};
    double max_vibration{2800.0};
    double max_pressure_psi{150.0};
    double nagumo_alpha{0.5};
};

class CbfSafetyGate {
public:
    [[nodiscard]] static GuardStatus evaluate(
        const plant_state_t& state,
        const plant_control_t& control,
        const CbfSafetyLimits& limits = {}) noexcept;
};

} // namespace ics_guard::defense