// File: /home/kami/ics-guard/src/protocols/iec104.cpp

#include "ics_guard/protocols/iec104.hpp"

namespace ics_guard::protocols {

GuardStatus Iec104Validator::validate(
    std::span<const uint8_t> frame,
    const Iec104Policy& policy) noexcept
{
    if (frame.size() < MIN_APCI_LEN || frame[0] != 0x68) {
        return GuardStatus::REJECT_MALFORMED_FRAME;
    }

    const uint8_t apdu_len = frame[1];
    if (apdu_len < 4 || (static_cast<size_t>(apdu_len) + 2U) > frame.size()) {
        return GuardStatus::REJECT_MALFORMED_FRAME;
    }

    const uint8_t ctrl0 = frame[2];

    // U-format (0x03) or S-format (0x01) supervisory frames pass
    if ((ctrl0 & 0x01) != 0) {
        return GuardStatus::PASS;
    }

    // I-format APDU carrying ASDU
    if (frame.size() < MIN_ASDU_LEN) {
        return GuardStatus::REJECT_MALFORMED_FRAME;
    }

    const uint8_t type_id = frame[6];
    const uint8_t cot = frame[8] & 0x3F; // Cause of Transmission (lower 6 bits)

    // Telecontrol Breaker / Actuator Commands (Single=45, Double=46, RegStep=47, Time-tagged=58..64)
    const bool is_telecontrol_cmd =
        (type_id == 45 || type_id == 46 || type_id == 47 || (type_id >= 58 && type_id <= 64));

    // COT=6 (Activation) or COT=8 (Deactivation) -> Industroyer / Industroyer2 Breaker Trip
    if (!policy.allow_telecontrol_activation && is_telecontrol_cmd && (cot == 6 || cot == 8)) {
        return GuardStatus::REJECT_IEC104_SPURIOUS_TRIP;
    }

    return GuardStatus::PASS;
}

} // namespace ics_guard::protocols