// File: /home/kami/ics-guard/benchmarks/bench_latency.cpp

#include "ics_guard/ics_guard.hpp"
#include <algorithm>
#include <iostream>
#include <numeric>
#include <vector>

int main()
{
    ics_guard::core::TscClock::calibrate();
    ics_guard::pipeline::GuardEngine engine;

    constexpr size_t ITERATIONS = 100'000;
    std::vector<double> samples_ns;
    samples_ns.reserve(ITERATIONS);

    const std::vector<uint8_t> modbus_read = {
        0x00, 0x01, 0x00, 0x00, 0x00, 0x06, 0x01, 0x03, 0x00, 0x00, 0x00, 0x04};

    for (size_t i = 0; i < ITERATIONS; ++i)
    {
        const uint64_t t0 = ics_guard::core::TscClock::rdtsc();
        const auto st = engine.process_modbus(modbus_read);
        const uint64_t t1 = ics_guard::core::TscClock::rdtsc();
        if (st == ics_guard::GuardStatus::PASS)
        {
            samples_ns.push_back(ics_guard::core::TscClock::cycles_to_nanoseconds(t1 - t0));
        }
    }

    std::sort(samples_ns.begin(), samples_ns.end());
    const double sum = std::accumulate(samples_ns.begin(), samples_ns.end(), 0.0);
    const double mean = sum / static_cast<double>(samples_ns.size());
    const double p99 = samples_ns[(samples_ns.size() * 99) / 100];

    std::cout << "================================================================================\n";
    std::cout << "  ICS-GUARD HARDWARE LATENCY BENCHMARK (100,000 MODBUS TCP FRAMES)\n";
    std::cout << "================================================================================\n";
    std::cout << "  TSC Frequency : " << ics_guard::core::TscClock::ghz() << " GHz\n";
    std::cout << "  Mean Latency  : " << mean << " ns\n";
    std::cout << "  P99 Latency   : " << p99 << " ns\n";
    std::cout << "  SLA Target    : < 120.0 ns -> " << (mean < 120.0 ? "PASS" : "FAIL") << "\n";
    std::cout << "================================================================================\n";

    return (mean < 120.0) ? 0 : 1;
}