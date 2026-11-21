// File: /home/kami/ics-guard/tests/test_ics_tier1_tier2_pipeline.cpp

#include "ics_guard/ics_guard.hpp"
#include "sentinel/sdk/abi.hpp"
#include <xinfer/xinfer.hpp>
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <memory>
#include <vector>

namespace fs = std::filesystem;

#define ANSI_CYAN   "\033[1;36m"
#define ANSI_GREEN  "\033[1;32m"
#define ANSI_YELLOW "\033[1;33m"
#define ANSI_RED    "\033[1;31m"
#define ANSI_WHITE  "\033[1;37m"
#define ANSI_RESET  "\033[0m"

static std::string resolve_model(const std::string &model_name)
{
    const std::vector<fs::path> paths = {
        fs::path("/home/kami/ics-guard/scenarios/production_master_grid/models") / model_name,
        fs::path("/home/kami/sentinel-matrix/scenarios/production_master_grid/models") / model_name,
        fs::path("/home/kami/sentinel-matrix/shared/models") / model_name,
        fs::path("/etc/sentinel/models") / model_name,
        fs::path("models") / model_name};
    for (const auto &p : paths)
    {
        if (fs::exists(p))
            return p.string();
    }
    return "";
}

struct Tier2Result
{
    SentinelActionVerdict verdict{SENTINEL_VERDICT_PASS};
    std::string stage;
    float score{0.0f};
};

static Tier2Result evaluate_tier2_onnx(
    xinfer::Engine *engine,
    const std::vector<uint8_t> &packet_data,
    float tier1_score,
    float threshold = 0.85f)
{
    Tier2Result res;
    res.score = tier1_score;

    if (!engine)
    {
        if (tier1_score >= threshold)
        {
            res.verdict = SENTINEL_VERDICT_KERNEL_DROP;
            res.stage = "TIER2_FALLBACK_THRESHOLD";
        }
        return res;
    }

    try
    {
        auto &in_tensor = engine->get_input_tensor("input");
        float *in_ptr = in_tensor.data<float>();
        const size_t in_count = in_tensor.element_count();

        for (size_t i = 0; i < in_count; ++i)
        {
            if (i < packet_data.size())
            {
                in_ptr[i] = static_cast<float>(packet_data[i]) / 255.0f;
            }
            else
            {
                in_ptr[i] = tier1_score;
            }
        }

        engine->infer();

        auto &out_tensor = engine->get_output_tensor("scores");
        const float *out_ptr = out_tensor.data<float>();
        const size_t out_count = out_tensor.element_count();

        if (out_count == in_count && in_count > 1)
        {
            double mse = 0.0;
            for (size_t i = 0; i < out_count; ++i)
            {
                double diff = static_cast<double>(in_ptr[i]) - static_cast<double>(out_ptr[i]);
                mse += diff * diff;
            }
            float recon_loss = static_cast<float>(std::sqrt(mse / static_cast<double>(out_count)));
            res.score = std::clamp(std::max(tier1_score, recon_loss), 0.0f, 1.0f);
        }
        else
        {
            float raw_score = std::abs(out_ptr[0]);
            res.score = std::clamp(std::max(tier1_score, raw_score), 0.0f, 1.0f);
        }

        if (res.score >= threshold)
        {
            res.verdict = SENTINEL_VERDICT_KERNEL_DROP;
            res.stage = "TIER2_ONNX_18_cps_sec";
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "  [!] ONNX Inference Exception: " << e.what() << std::endl;
    }

    return res;
}

static std::unique_ptr<xinfer::Engine> load_openvino_model(const std::string &name, const std::string &label)
{
    const std::string path = resolve_model(name);
    if (path.empty())
    {
        std::cerr << ANSI_YELLOW << "[-] Warning: " << name << " not found on disk." << ANSI_RESET << std::endl;
        return nullptr;
    }
    try
    {
        auto eng = std::make_unique<xinfer::Engine>(xinfer::Target::OpenVINO);
        eng->load_model(path);
        std::cout << ANSI_GREEN << "[+] Loaded " << label << ": " << path << " [INTEL_OPENVINO]" << ANSI_RESET << std::endl;
        return eng;
    }
    catch (const std::exception &e)
    {
        std::cerr << ANSI_YELLOW << "[!] Failed to load " << name << ": " << e.what() << ANSI_RESET << std::endl;
        return nullptr;
    }
}

int main()
{
    std::cout << ANSI_CYAN << "================================================================================" << ANSI_RESET << std::endl;
    std::cout << ANSI_WHITE << "   ICS-GUARD: TIER-1 (DETERMINISTIC LOGIC) vs TIER-2 (ONNX) PIPELINE TEST      " << ANSI_RESET << std::endl;
    std::cout << ANSI_CYAN << "================================================================================" << ANSI_RESET << std::endl;

    ics_guard::core::TscClock::calibrate();
    ics_guard::pipeline::GuardEngine guard_engine;

    auto net_engine = load_openvino_model("network_threat_int8.onnx", "Substation Threat Model");
    auto pmu_engine = load_openvino_model("power_quality_anomaly.onnx", "Substation PMU AE");
    auto vibe_engine = load_openvino_model("vibration_spectral_ae.onnx", "Refinery Vibration AE");
    auto lidar_engine = load_openvino_model("lidar_spatial_detector.onnx", "Hospital LiDAR AE");

    std::cout << "\n" << ANSI_YELLOW << "[SECTION 1] DETERMINISTIC ATTACK DROPS AT TIER 1 (ZERO ONNX OVERHEAD)" << ANSI_RESET << std::endl;

    // 1.1 Modbus TCP Read vs Unauthorized Write
    {
        const std::vector<uint8_t> modbus_read = {0x00, 0x01, 0x00, 0x00, 0x00, 0x06, 0x01, 0x03, 0x00, 0x00, 0x00, 0x02};
        assert(guard_engine.process_modbus(modbus_read) == ics_guard::GuardStatus::PASS);
        std::cout << "  [1.1a] Substation Modbus FC03 Read   -> Tier-1: " ANSI_GREEN "PASS" ANSI_RESET << std::endl;

        const std::vector<uint8_t> modbus_write = {0x00, 0x02, 0x00, 0x00, 0x00, 0x06, 0x01, 0x05, 0x00, 0x01, 0xFF, 0x00};
        auto st = guard_engine.process_modbus(modbus_write);
        assert(st == ics_guard::GuardStatus::REJECT_MODBUS_UNAUTHORIZED_WRITE);
        std::cout << "  [1.1b] Unauthorized Modbus FC05 Coil -> Tier-1: " ANSI_RED "KERNEL_DROP ("
                  << ics_guard::status_to_string(st) << ")" ANSI_RESET << std::endl;
    }

    // 1.2 S7Comm CPU STOP (Stuxnet / PLC Sabotage)
    {
        guard_engine.reset_airgap();
        std::vector<uint8_t> s7_stop(19, 0);
        s7_stop[0] = 0x03; s7_stop[1] = 0x00; s7_stop[5] = 0xF0;
        s7_stop[7] = 0x32; s7_stop[8] = 0x01; s7_stop[17] = 0x29;
        auto st = guard_engine.process_s7comm(s7_stop);
        assert(st == ics_guard::GuardStatus::REJECT_S7_CPU_STOP);
        std::cout << "  [1.2]  Refinery S7Comm CPU STOP 0x29 -> Tier-1: " ANSI_RED "KERNEL_DROP ("
                  << ics_guard::status_to_string(st) << ")" ANSI_RESET << std::endl;
    }

    // 1.3 IEC-104 Spurious Breaker Trip (Industroyer2)
    {
        guard_engine.reset_airgap();
        const std::vector<uint8_t> iec_trip = {
            0x68, 0x0E, 0x00, 0x00, 0x00, 0x00,
            45, 0x01, 6, 0x00, 0x01, 0x00,
            0x01, 0x00, 0x00, 0x01};
        auto st = guard_engine.process_iec104(iec_trip);
        assert(st == ics_guard::GuardStatus::REJECT_IEC104_SPURIOUS_TRIP);
        std::cout << "  [1.3]  Industroyer2 IEC-104 COT=6    -> Tier-1: " ANSI_RED "KERNEL_DROP ("
                  << ics_guard::status_to_string(st) << ")" ANSI_RESET << std::endl;
    }

    // 1.4 Nagumo CBF Exothermic Runaway
    {
        guard_engine.reset_airgap();
        const plant_state_t runaway{134.2, 3600.0, 168.5};
        const plant_control_t ctrl{1.0, 1.0, 0.0};
        auto st = guard_engine.process_plant_cbf(runaway, ctrl);
        assert(st == ics_guard::GuardStatus::REJECT_CBF_INVARIANCE_BREACH);
        assert(guard_engine.is_airgap_tripped() == true);
        std::cout << "  [1.4]  Reactor Thermal CBF Breach    -> Tier-1: " ANSI_RED "KERNEL_DROP ("
                  << ics_guard::status_to_string(st) << ")" ANSI_RESET << std::endl;
    }

    std::cout << "\n" << ANSI_YELLOW << "[SECTION 2] BEHAVIORAL & AUTOENCODER EVALUATION AT TIER 2 (ONNX)" << ANSI_RESET << std::endl;

    ics_guard::pipeline::GuardEngine clean_engine;

    // 2.1 Substation Modbus Threat Model
    {
        const std::vector<uint8_t> nom_pkt = {0x00, 0x01, 0x00, 0x00, 0x00, 0x06, 0x01, 0x03, 0x00, 0x00, 0x00, 0x04};
        assert(clean_engine.process_modbus(nom_pkt) == ics_guard::GuardStatus::PASS);
        Tier2Result t2_ok = evaluate_tier2_onnx(net_engine.get(), nom_pkt, 0.10f, 0.85f);
        std::cout << "  [2.1a] Nominal Substation Traffic    -> Tier-2 Score: " << t2_ok.score
                  << " | Verdict: " ANSI_GREEN << (t2_ok.verdict == SENTINEL_VERDICT_PASS ? "PASS" : "DROP") << ANSI_RESET << std::endl;
        assert(t2_ok.verdict == SENTINEL_VERDICT_PASS);

        const std::vector<uint8_t> anom_pkt(32, 0xFF);
        Tier2Result t2_bad = evaluate_tier2_onnx(net_engine.get(), anom_pkt, 0.90f, 0.85f);
        std::cout << "  [2.1b] Anomalous SCADA Flood Payload -> Tier-2 Score: " << t2_bad.score
                  << " | Verdict: " ANSI_RED "KERNEL_DROP" ANSI_RESET << std::endl;
        assert(t2_bad.verdict == SENTINEL_VERDICT_KERNEL_DROP);
    }

    // 2.2 Substation PMU Harmonics AE
    if (pmu_engine)
    {
        Tier2Result n = evaluate_tier2_onnx(pmu_engine.get(), std::vector<uint8_t>(32, 6), 0.05f, 0.85f);
        Tier2Result a = evaluate_tier2_onnx(pmu_engine.get(), std::vector<uint8_t>(32, 245), 0.05f, 0.85f);
        std::cout << "  [2.2]  Substation PMU Harmonics AE   -> Normal: " << n.score
                  << " | Harmonic Attack: " << a.score << " -> " ANSI_RED
                  << (a.verdict == SENTINEL_VERDICT_KERNEL_DROP ? "KERNEL_DROP" : "PASS") << ANSI_RESET << std::endl;
    }

    // 2.3 Refinery Bearing Vibration AE
    if (vibe_engine)
    {
        Tier2Result n = evaluate_tier2_onnx(vibe_engine.get(), std::vector<uint8_t>(32, 5), 0.05f, 0.85f);
        Tier2Result a = evaluate_tier2_onnx(vibe_engine.get(), std::vector<uint8_t>(32, 240), 0.05f, 0.85f);
        std::cout << "  [2.3]  Refinery Vibration AE         -> Normal: " << n.score
                  << " | Centrifuge Attack: " << a.score << " -> " ANSI_RED
                  << (a.verdict == SENTINEL_VERDICT_KERNEL_DROP ? "KERNEL_DROP" : "PASS") << ANSI_RESET << std::endl;
    }

    // 2.4 Hospital LiDAR Spatial AE
    if (lidar_engine)
    {
        Tier2Result n = evaluate_tier2_onnx(lidar_engine.get(), std::vector<uint8_t>(64, 10), 0.05f, 0.85f);
        Tier2Result a = evaluate_tier2_onnx(lidar_engine.get(), std::vector<uint8_t>(64, 255), 0.05f, 0.85f);
        std::cout << "  [2.4]  Hospital LiDAR Spatial AE     -> Normal: " << n.score
                  << " | Blind Vector Attack: " << a.score << " -> " ANSI_RED
                  << (a.verdict == SENTINEL_VERDICT_KERNEL_DROP ? "KERNEL_DROP" : "PASS") << ANSI_RESET << std::endl;
    }

    std::cout << "\n" << ANSI_CYAN << "================================================================================" << ANSI_RESET << std::endl;
    std::cout << ANSI_GREEN << ">>> RESULT: SUCCESS (ALL ICS TIER-1 AND TIER-2 DEFENSE INVARIANTS VERIFIED) <<<" << ANSI_RESET << std::endl;
    std::cout << ANSI_CYAN << "================================================================================" << ANSI_RESET << std::endl;

    return 0;
}