// File: /home/kami/ics-guard/src/defense/cbf_safety_gate.cpp

#include "ics_guard/defense/cbf_safety_gate.hpp"

namespace ics_guard::defense {

GuardStatus CbfSafetyGate::evaluate(
    const plant_state_t& state,
    const plant_control_t& control,
    const CbfSafetyLimits& limits) noexcept
{
    if (!plant_state_is_safe(&state, limits.max_temp_c, limits.max_vibration, limits.max_pressure_psi)) {
        return GuardStatus::REJECT_CBF_INVARIANCE_BREACH;
    }

    if (!plant_cbf_check_invariance(&state, &control, limits.max_temp_c, limits.nagumo_alpha)) {
        return GuardStatus::REJECT_CBF_INVARIANCE_BREACH;
    }

    return GuardStatus::PASS;
}

} // namespace ics_guard::defense