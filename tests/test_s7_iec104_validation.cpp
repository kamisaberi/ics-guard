// File: /home/kami/ics-guard/tests/test_s7_iec104_validation.cpp

#include "ics_guard/pipeline/guard_engine.hpp"
#include <cassert>
#include <iostream>
#include <vector>

int main()
{
    std::cout << "[TEST] S7Comm CPU STOP & IEC-104 Spurious Breaker Trip Validation... ";

    ics_guard::pipeline::GuardEngine engine;

    // 1. S7 COTP Connection Request (0xE0) -> PASS
    const std::vector<uint8_t> cotp_cr = {0x03, 0x00, 0x00, 0x16, 0x11, 0xE0, 0x00, 0x00, 0x00, 0x01, 0x00};
    assert(engine.process_s7comm(cotp_cr) == ics_guard::GuardStatus::PASS);

    // 2. S7Comm CPU STOP (0x29) -> REJECT_S7_CPU_STOP
    std::vector<uint8_t> s7_stop(19, 0);
    s7_stop[0] = 0x03; s7_stop[1] = 0x00; s7_stop[5] = 0xF0;
    s7_stop[7] = 0x32; s7_stop[8] = 0x01; s7_stop[17] = 0x29;
    assert(engine.process_s7comm(s7_stop) == ics_guard::GuardStatus::REJECT_S7_CPU_STOP);

    // 3. Stuxnet S7 Write Var (0x05) -> REJECT_S7_UNAUTHORIZED_WRITE
    engine.reset_airgap();
    std::vector<uint8_t> s7_write(19, 0);
    s7_write[0] = 0x03; s7_write[1] = 0x00; s7_write[5] = 0xF0;
    s7_write[7] = 0x32; s7_write[8] = 0x01; s7_write[17] = 0x05;
    assert(engine.process_s7comm(s7_write) == ics_guard::GuardStatus::REJECT_S7_UNAUTHORIZED_WRITE);

    // 4. IEC-104 TESTFR U-Frame -> PASS
    engine.reset_airgap();
    const std::vector<uint8_t> iec_testfr = {0x68, 0x04, 0x43, 0x00, 0x00, 0x00};
    assert(engine.process_iec104(iec_testfr) == ics_guard::GuardStatus::PASS);

    // 5. Industroyer2 IEC-104 Type 45 Single Command COT=6 -> REJECT_IEC104_SPURIOUS_TRIP
    const std::vector<uint8_t> iec_trip = {
        0x68, 0x0E, 0x00, 0x00, 0x00, 0x00,
        45, 0x01, 6, 0x00, 0x01, 0x00,
        0x01, 0x00, 0x00, 0x01};
    assert(engine.process_iec104(iec_trip) == ics_guard::GuardStatus::REJECT_IEC104_SPURIOUS_TRIP);

    std::cout << "PASSED\n";
    return 0;
}