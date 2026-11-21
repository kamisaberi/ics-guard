// File: /home/kami/ics-guard/src/pipeline/guard_engine.cpp

#include "ics_guard/pipeline/guard_engine.hpp"

namespace ics_guard::pipeline {

GuardStatus GuardEngine::process_modbus(
    std::span<const uint8_t> frame,
    const protocols::ModbusPolicy& policy) noexcept
{
    if (airgap_.is_tripped()) {
        return GuardStatus::ACTUATOR_AIRGAP_TRIPPED;
    }
    const GuardStatus st = protocols::ModbusTcpValidator::validate(frame, policy);
    if (st != GuardStatus::PASS) {
        airgap_.trip();
    }
    return st;
}

GuardStatus GuardEngine::process_s7comm(
    std::span<const uint8_t> frame,
    const protocols::S7Policy& policy) noexcept
{
    if (airgap_.is_tripped()) {
        return GuardStatus::ACTUATOR_AIRGAP_TRIPPED;
    }
    const GuardStatus st = protocols::S7CommValidator::validate(frame, policy);
    if (st != GuardStatus::PASS) {
        airgap_.trip();
    }
    return st;
}

GuardStatus GuardEngine::process_iec104(
    std::span<const uint8_t> frame,
    const protocols::Iec104Policy& policy) noexcept
{
    if (airgap_.is_tripped()) {
        return GuardStatus::ACTUATOR_AIRGAP_TRIPPED;
    }
    const GuardStatus st = protocols::Iec104Validator::validate(frame, policy);
    if (st != GuardStatus::PASS) {
        airgap_.trip();
    }
    return st;
}

GuardStatus GuardEngine::process_synchrophasor(
    const SynchrophasorSample& sample,
    const protocols::SynchrophasorLimits& limits) noexcept
{
    if (airgap_.is_tripped()) {
        return GuardStatus::ACTUATOR_AIRGAP_TRIPPED;
    }
    const GuardStatus st = protocols::SynchrophasorValidator::validate(sample, limits);
    if (st != GuardStatus::PASS) {
        airgap_.trip();
    }
    return st;
}

GuardStatus GuardEngine::process_plant_cbf(
    const plant_state_t& state,
    const plant_control_t& control,
    const defense::CbfSafetyLimits& limits) noexcept
{
    if (airgap_.is_tripped()) {
        return GuardStatus::ACTUATOR_AIRGAP_TRIPPED;
    }
    const GuardStatus st = defense::CbfSafetyGate::evaluate(state, control, limits);
    if (st != GuardStatus::PASS) {
        airgap_.trip();
    }
    return st;
}

} // namespace ics_guard::pipeline