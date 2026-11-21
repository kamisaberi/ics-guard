// File: /home/kami/ics-guard/tests/test_modbus_validation.cpp

#include "ics_guard/pipeline/guard_engine.hpp"
#include <cassert>
#include <iostream>
#include <vector>

int main()
{
    std::cout << "[TEST] Modbus TCP Function Code & Write-Lock Validation... ";

    ics_guard::pipeline::GuardEngine engine;

    // 1. Valid Read Holding Registers (FC 0x03)
    const std::vector<uint8_t> read_pkt = {0x00, 0x01, 0x00, 0x00, 0x00, 0x06, 0x01, 0x03, 0x00, 0x00, 0x00, 0x02};
    assert(engine.process_modbus(read_pkt) == ics_guard::GuardStatus::PASS);

    // 2. Illegal Diagnostic Function Code (FC 0x08)
    const std::vector<uint8_t> diag_pkt = {0x00, 0x02, 0x00, 0x00, 0x00, 0x06, 0x01, 0x08, 0x00, 0x00, 0x00, 0x00};
    assert(engine.process_modbus(diag_pkt) == ics_guard::GuardStatus::REJECT_MODBUS_ILLEGAL_FC);
    assert(engine.is_airgap_tripped() == true);

    // 3. Subsequent frame blocked by latched air-gap until reset
    assert(engine.process_modbus(read_pkt) == ics_guard::GuardStatus::ACTUATOR_AIRGAP_TRIPPED);

    engine.reset_airgap();
    const std::vector<uint8_t> write_pkt = {0x00, 0x03, 0x00, 0x00, 0x00, 0x06, 0x01, 0x05, 0x00, 0x01, 0xFF, 0x00};
    assert(engine.process_modbus(write_pkt) == ics_guard::GuardStatus::REJECT_MODBUS_UNAUTHORIZED_WRITE);

    std::cout << "PASSED\n";
    return 0;
}