// File: /home/kami/ics-guard/tests/test_sentinel_plugin.cpp

#include "sentinel/sdk/abi.hpp"
#include <cassert>
#include <cstring>
#include <dlfcn.h>
#include <filesystem>
#include <iostream>
#include <vector>

static int g_log_count = 0;
static int g_metric_count = 0;

static void mock_log_message(int level, const char *sender, const char *msg)
{
    std::cout << "  [HOST_LOG level=" << level << " sender=" << sender << "] " << msg << "\n";
    g_log_count++;
}

static void mock_emit_metric_counter(const char *metric_name, uint64_t delta)
{
    std::cout << "  [HOST_METRIC] " << metric_name << " (+" << delta << ")\n";
    g_metric_count += static_cast<int>(delta);
}

int main()
{
    std::cout << "[TEST] blackbox-sentinel Native ICS Plugin ABI Loader Integration...\n";

    const char *plugin_path = std::filesystem::exists("./ics_guard_dissector_plugin.so")
                                  ? "./ics_guard_dissector_plugin.so"
                                  : "/usr/local/lib/blackbox/ics_guard_dissector_plugin.so";

    void *handle = dlopen(plugin_path, RTLD_NOW | RTLD_LOCAL);
    if (!handle)
    {
        std::cerr << "[-] Error: Failed to dlopen plugin (" << plugin_path << "): " << dlerror() << "\n";
        return 1;
    }

    using GetDescriptorFn = const SentinelPluginDescriptor *(*)(void);
    auto get_desc = reinterpret_cast<GetDescriptorFn>(dlsym(handle, "sentinel_plugin_get_descriptor"));
    assert(get_desc != nullptr);

    const SentinelPluginDescriptor *desc = get_desc();
    assert(desc != nullptr);
    assert(desc->magic == SENTINEL_SDK_MAGIC);
    assert(desc->sdk_version_major == SENTINEL_SDK_VERSION_MAJOR);
    assert(std::string(desc->plugin_id) == "org.aryorithm.plugin.ics_guard");

    std::cout << "  [+] Plugin descriptor verified: " << desc->plugin_name << "\n";

    SentinelHostInterface host{};
    host.engine_version = 1;
    host.log_message = mock_log_message;
    host.emit_metric_counter = mock_emit_metric_counter;

    assert(desc->init_fn != nullptr);
    assert(desc->init_fn(&host) == 0);
    assert(g_log_count >= 1);

    // 1. Valid Modbus TCP Read (FC 0x03) -> PASS
    std::vector<uint8_t> modbus_read = {0x00, 0x01, 0x00, 0x00, 0x00, 0x06, 0x01, 0x03, 0x00, 0x00, 0x00, 0x02};
    SentinelRawPacket pkt_read{};
    pkt_read.data = modbus_read.data();
    pkt_read.length = modbus_read.size();
    pkt_read.timestamp_ns = 10'000'000'000ULL;
    pkt_read.network_proto = 0x0800;
    pkt_read.transport_proto = 6;

    SentinelDissectorResult res1 = desc->dissect_fn(&pkt_read);
    assert(res1.verdict == SENTINEL_VERDICT_PASS);
    std::cout << "  [+] Valid Modbus FC03 verdict: PASS\n";

    // 2. Unauthorized Modbus Coil Write (FC 0x05) -> KERNEL_DROP
    std::vector<uint8_t> modbus_write = {0x00, 0x02, 0x00, 0x00, 0x00, 0x06, 0x01, 0x05, 0x00, 0x01, 0xFF, 0x00};
    SentinelRawPacket pkt_write{};
    pkt_write.data = modbus_write.data();
    pkt_write.length = modbus_write.size();
    pkt_write.timestamp_ns = 10'100'000'000ULL;
    pkt_write.network_proto = 0x0800;
    pkt_write.transport_proto = 6;

    const int pre_drops = g_metric_count;
    SentinelDissectorResult res2 = desc->dissect_fn(&pkt_write);
    assert(res2.verdict == SENTINEL_VERDICT_KERNEL_DROP);
    assert(res2.threat_score >= 900);
    assert(res2.rule_id == 0x0502);
    assert(g_metric_count > pre_drops);
    std::cout << "  [+] Unauthorized Modbus FC05 verdict: KERNEL_DROP (Rule: 0x" << std::hex << res2.rule_id << std::dec << ")\n";

    assert(desc->shutdown_fn != nullptr);
    desc->shutdown_fn();
    dlclose(handle);

    std::cout << ">>> RESULT: SUCCESS (blackbox-sentinel Native ICS Plugin Verified) <<<\n";
    return 0;
}