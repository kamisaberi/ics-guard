// File: /home/kami/ics-guard/tools/aryo_ics/main.cpp

#include "ics_guard/ics_guard.hpp"
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>
#include <sentinel/fs/PathResolver.hpp>
#include <filesystem>

static void print_help()
{
    std::cout << R"(================================================================================
ARYO ICS: INDUSTRIAL SCADA, SUBSTATION & REFINERY DEFENSE (ics-guard)
================================================================================
Standards: IEC 62443-4-2, Modbus TCP, Siemens S7Comm, IEC 60870-5-104, IEEE C37.118
Invariant: Deterministic sub-120 ns verification with zero runtime heap allocation.

Usage:
  aryo ics <action> [arguments]

Available Actions:
  bench                   Run the 100,000-sample hardware latency benchmark
  verify-modbus           Test deterministic Modbus TCP read/write validation
  verify-s7               Test Siemens S7Comm CPU STOP interception (0x29)
  verify-iec104           Test IEC 60870-5-104 breaker trip interception (COT=6)
  verify-cbf              Test Nagumo CBF thermal safety invariance gate
  airgap-status           Inspect real-time actuator tripwire latch state
  grid-up                 Deploy the Production Master Grid digital twin
  grid-down               Teardown the Production Master Grid mesh
  grid-status             Show running Production Master Grid containers
  --plugin-desc           Print single-line extension metadata for master CLI

Examples:
  aryo ics bench
  aryo ics verify-iec104
  aryo ics grid-up
  aryo ics grid-status
  aryo ics grid-down
)" << std::endl;
}




