// File: /home/kami/ics-guard/tools/ics_guard_ctl/main.cpp

#include "ics_guard/ics_guard.hpp"
#include <iostream>
#include <string>
#include <vector>

int main(int argc, char *argv[])
{
    ics_guard::core::TscClock::calibrate();
    ics_guard::pipeline::GuardEngine engine;

    if (argc > 1 && std::string(argv[1]) == "--help")
    {
        std::cout << "Usage: ics-guard-ctl [--verify-all]\n";
        return 0;
    }

    const std::vector<uint8_t> modbus_read = {0x00, 0x01, 0x00, 0x00, 0x00, 0x06, 0x01, 0x03, 0x00, 0x00, 0x00, 0x01};
    const auto st = engine.process_modbus(modbus_read);

    std::cout << "[ics-guard-ctl] TSC Clock : " << ics_guard::core::TscClock::ghz() << " GHz\n";
    std::cout << "[ics-guard-ctl] Self-Test : " << ics_guard::status_to_string(st) << "\n";
    return (st == ics_guard::GuardStatus::PASS) ? 0 : 1;
}