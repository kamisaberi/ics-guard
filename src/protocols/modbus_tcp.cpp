// File: /home/kami/ics-guard/src/protocols/modbus_tcp.cpp

#include "ics_guard/protocols/modbus_tcp.hpp"

namespace ics_guard::protocols {

GuardStatus ModbusTcpValidator::validate(
    std::span<const uint8_t> frame,
    const ModbusPolicy& policy) noexcept
{
    if (frame.size() < MBAP_HEADER_LEN) {
        return GuardStatus::REJECT_MALFORMED_FRAME;
    }

    // MBAP Protocol Identifier must be 0x0000 for Modbus TCP
    const uint16_t proto_id = (static_cast<uint16_t>(frame[2]) << 8) | frame[3];
    const uint16_t mbap_len = (static_cast<uint16_t>(frame[4]) << 8) | frame[5];
    if (proto_id != 0x0000 || mbap_len < 2 || (mbap_len + 6U) > frame.size()) {
        return GuardStatus::REJECT_MALFORMED_FRAME;
    }

    const uint8_t fc = frame[7];

    // Standard safe read function codes (Coils, Discrete Inputs, Holding/Input Registers)
    if (fc == 0x01 || fc == 0x02 || fc == 0x03 || fc == 0x04) {
        return GuardStatus::PASS;
    }

    // Write function codes (Single/Multiple Coil & Register Writes, Read/Write Multiple)
    if (fc == 0x05 || fc == 0x06 || fc == 0x0F || fc == 0x10 || fc == 0x17) {
        if (!policy.allow_writes) {
            return GuardStatus::REJECT_MODBUS_UNAUTHORIZED_WRITE;
        }
        if (frame.size() >= 10) {
            const uint16_t addr = (static_cast<uint16_t>(frame[8]) << 8) | frame[9];
            const uint16_t reg_num = static_cast<uint16_t>(40001U + addr);
            if (reg_num >= policy.protected_reg_start && reg_num <= policy.protected_reg_end) {
                return GuardStatus::REJECT_MODBUS_UNAUTHORIZED_WRITE;
            }
        }
        return GuardStatus::PASS;
    }

    // Diagnostics / Encapsulated / Proprietary / Illegal Function Codes (e.g. 0x08, 0x2B, >0x7F)
    return GuardStatus::REJECT_MODBUS_ILLEGAL_FC;
}

} // namespace ics_guard::protocols