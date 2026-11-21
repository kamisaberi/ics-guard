// File: /home/kami/ics-guard/src/protocols/s7comm.cpp

#include "ics_guard/protocols/s7comm.hpp"

namespace ics_guard::protocols {

GuardStatus S7CommValidator::validate(
    std::span<const uint8_t> frame,
    const S7Policy& policy) noexcept
{
    if (frame.size() < MIN_TPKT_COTP_LEN) {
        return GuardStatus::REJECT_MALFORMED_FRAME;
    }

    // TPKT Version 3 (0x03), Reserved (0x00)
    if (frame[0] != 0x03 || frame[1] != 0x00) {
        return GuardStatus::REJECT_MALFORMED_FRAME;
    }

    // COTP Connection Request (0xE0) / Connect Confirm (0xD0) handshake frame
    if (frame[5] == 0xE0 || frame[5] == 0xD0) {
        return GuardStatus::PASS;
    }

    // S7Comm PDU Header (Protocol ID 0x32 at offset 7)
    if (frame.size() < MIN_S7_PDU_LEN || frame[7] != 0x32) {
        return GuardStatus::REJECT_MALFORMED_FRAME;
    }

    const uint8_t rosctr = frame[8]; // 0x01 = Job Request, 0x07 = Userdata
    const uint8_t func = frame[17];

    // Block CPU STOP (0x29) and PLC Control / Program Invocation (0x28)
    if (policy.block_plc_control && (func == 0x29 || func == 0x28)) {
        return GuardStatus::REJECT_S7_CPU_STOP;
    }

    // Block unauthorized Write Var (0x05) or Block Download (0x1A..0x1C) on Job Requests (Stuxnet vector)
    if (!policy.allow_db_writes && rosctr == 0x01 &&
        (func == 0x05 || func == 0x1A || func == 0x1B || func == 0x1C)) {
        return GuardStatus::REJECT_S7_UNAUTHORIZED_WRITE;
    }

    return GuardStatus::PASS;
}

} // namespace ics_guard::protocols