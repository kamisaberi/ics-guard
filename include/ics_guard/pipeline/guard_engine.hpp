// File: /home/kami/ics-guard/include/ics_guard/pipeline/guard_engine.hpp

#pragma once

#include "ics_guard/core/types.hpp"
#include "ics_guard/protocols/modbus_tcp.hpp"
#include "ics_guard/protocols/s7comm.hpp"
#include "ics_guard/protocols/iec104.hpp"
#include "ics_guard/protocols/synchrophasor.hpp"
#include "ics_guard/defense/cbf_safety_gate.hpp"
#include "ics_guard/defense/actuator_airgap.hpp"
#include <span>

namespace ics_guard::pipeline {

class GuardEngine {
public:
    [[nodiscard]] GuardStatus process_modbus(
        std::span<const uint8_t> frame,
        const protocols::ModbusPolicy& policy = {}) noexcept;

    [[nodiscard]] GuardStatus process_s7comm(
        std::span<const uint8_t> frame,
        const protocols::S7Policy& policy = {}) noexcept;

    [[nodiscard]] GuardStatus process_iec104(
        std::span<const uint8_t> frame,
        const protocols::Iec104Policy& policy = {}) noexcept;

    [[nodiscard]] GuardStatus process_synchrophasor(
        const SynchrophasorSample& sample,
        const protocols::SynchrophasorLimits& limits = {}) noexcept;

    [[nodiscard]] GuardStatus process_plant_cbf(
        const plant_state_t& state,
        const plant_control_t& control,
        const defense::CbfSafetyLimits& limits = {}) noexcept;

    [[nodiscard]] bool is_airgap_tripped() const noexcept { return airgap_.is_tripped(); }
    void reset_airgap() noexcept { airgap_.reset(); }

private:
    defense::ActuatorAirGap airgap_;
};

} // namespace ics_guard::pipeline