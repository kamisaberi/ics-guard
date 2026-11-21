// File: /home/kami/ics-guard/tools/sentinel_plugin/ics_guard_plugin.cpp

#include "sentinel/sdk/abi.hpp"
#include "ics_guard/ics_guard.hpp"
#include <algorithm>
#include <cstring>
#include <memory>
#include <span>
#include <string>

static const SentinelHostInterface *g_host = nullptr;
static std::unique_ptr<ics_guard::pipeline::GuardEngine> g_guard_engine;

static int plugin_init(const SentinelHostInterface *host_iface)
{
    g_host = host_iface;
    g_guard_engine = std::make_unique<ics_guard::pipeline::GuardEngine>();
    ics_guard::core::TscClock::calibrate();

    if (g_host && g_host->log_message)
    {
        g_host->log_message(1, "ICSGuard",
                            "Industrial SCADA & OT Telecontrol Defense Plugin Initialized (< 120 ns SLA).");
    }
    return 0;
}

static void plugin_shutdown()
{
    g_guard_engine.reset();
    g_host = nullptr;
}

static SentinelDissectorResult plugin_dissect(const SentinelRawPacket *packet)
{
    SentinelDissectorResult res{};
    res.verdict = SENTINEL_VERDICT_PASS;
    res.threat_score = 0;
    res.rule_id = 0;
    res.semantic_tags = 0;
    res.threat_name[0] = '\0';

    if (!packet || !packet->data || packet->length < 6 || !g_guard_engine)
    {
        return res;
    }

    // Only inspect TCP SCADA streams (6) or raw test frames (0)
    if (packet->transport_proto != 6 && packet->transport_proto != 0)
    {
        return res;
    }

    const uint8_t *payload_ptr = packet->data;
    size_t payload_len = packet->length;
    uint16_t dst_port = 0;

    // Strip Ethernet + IPv4 + TCP headers if full L2 frame is passed
    if (packet->length >= 54 &&
        packet->data[12] == 0x08 && packet->data[13] == 0x00 &&
        packet->data[23] == 6)
    {
        const size_t ihl = static_cast<size_t>(packet->data[14] & 0x0F) * 4U;
        const size_t tcp_off = 14U + ihl;
        if (ihl >= 20 && tcp_off + 20U <= packet->length)
        {
            dst_port = (static_cast<uint16_t>(packet->data[tcp_off + 2]) << 8) |
                       packet->data[tcp_off + 3];
            const size_t tcp_hlen = static_cast<size_t>((packet->data[tcp_off + 12] >> 4) & 0x0F) * 4U;
            const size_t payload_off = tcp_off + tcp_hlen;
            if (tcp_hlen < 20 || payload_off >= packet->length)
            {
                return res; // Pure TCP ACK/SYN with empty payload
            }
            payload_ptr = packet->data + payload_off;
            payload_len = packet->length - payload_off;
        }
    }

    std::span<const uint8_t> frame(payload_ptr, payload_len);
    ics_guard::GuardStatus st = ics_guard::GuardStatus::PASS;
    uint32_t matched_rule_id = 0;

    // 1. Modbus TCP (Port 502 or MBAP Proto ID 0x0000)
    const bool is_modbus = (dst_port == 502) ||
                           (dst_port == 0 && payload_len >= 8 &&
                            payload_ptr[2] == 0x00 && payload_ptr[3] == 0x00);

    // 2. Siemens S7Comm (Port 102 or TPKT 0x03 0x00 + COTP/0x32)
    const bool is_s7 = (dst_port == 102) ||
                       (dst_port == 0 && payload_len >= 7 &&
                        payload_ptr[0] == 0x03 && payload_ptr[1] == 0x00);

    // 3. IEC 60870-5-104 (Port 2404 or APCI Start Byte 0x68)
    const bool is_iec104 = (dst_port == 2404) ||
                           (dst_port == 0 && payload_len >= 6 &&
                            payload_ptr[0] == 0x68);

    if (is_modbus)
    {
        st = g_guard_engine->process_modbus(frame);
        matched_rule_id = 0x0502;
    }
    else if (is_s7)
    {
        st = g_guard_engine->process_s7comm(frame);
        matched_rule_id = 0x0102;
    }
    else if (is_iec104)
    {
        st = g_guard_engine->process_iec104(frame);
        matched_rule_id = 0x2404;
    }

    if (st != ics_guard::GuardStatus::PASS)
    {
        res.verdict = SENTINEL_VERDICT_KERNEL_DROP;
        res.threat_score = 990;
        res.rule_id = matched_rule_id;
        std::strncpy(res.threat_name, ics_guard::status_to_string(st).data(), sizeof(res.threat_name) - 1);

        if (g_host)
        {
            if (g_host->log_message)
            {
                std::string msg = "ICS DEFENSE TRIPWIRE: Dropped industrial command (" +
                                  std::string(res.threat_name) + ")";
                g_host->log_message(3, "ICSGuard", msg.c_str());
            }
            if (g_host->emit_metric_counter)
            {
                g_host->emit_metric_counter("ics_telecontrol_airgap_trips", 1);
            }
        }
        return res;
    }

    // Baseline score passed forward to Stage 2/3 ModelRouter ONNX chain
    res.threat_score = static_cast<uint32_t>(std::min<size_t>(payload_len * 2U, 250U));
    return res;
}

static const SentinelPluginDescriptor g_descriptor = {
    .magic = SENTINEL_SDK_MAGIC,
    .sdk_version_major = SENTINEL_SDK_VERSION_MAJOR,
    .sdk_version_minor = SENTINEL_SDK_VERSION_MINOR,
    .tier = SENTINEL_TIER_NATIVE_CPP,
    .plugin_id = "org.aryorithm.plugin.ics_guard",
    .plugin_name = "Industrial SCADA, Substation & Refinery Deterministic Defense Engine",
    .plugin_version = "1.0.0",
    .target_protocol = "MODBUS_S7COMM_IEC104_C37118",
    .default_port = 502,
    .reserved = 0,
    .init_fn = plugin_init,
    .shutdown_fn = plugin_shutdown,
    .dissect_fn = plugin_dissect};

extern "C" ARY_SDK_API const SentinelPluginDescriptor *sentinel_plugin_get_descriptor(void)
{
    return &g_descriptor;
}