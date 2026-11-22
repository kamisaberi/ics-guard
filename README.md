# ics-guard

**Deterministic Sub-120 ns Industrial SCADA, Substation & Critical Infrastructure Defense Engine**

[![Organization](https://img.shields.io/badge/Aryorithm%20Technologies%20B.V.-Amsterdam-00E5FF?style=flat-square)](https://aryorithm.com)
[![Preprint DOI](https://img.shields.io/badge/Zenodo%20DOI-10.5281%2Fzenodo.23598779-00FFA3?style=flat-square)](https://doi.org/10.5281/zenodo.23598779)
[![C++ Standard](https://img.shields.io/badge/C%2B%2B-20%20Zero--Alloc-blue?style=flat-square)](#)
[![Standards](https://img.shields.io/badge/Standards-IEC%2062443%20%7C%20EU%20NIS2-purple?style=flat-square)](#)

`ics-guard` is the Critical Infrastructure & Operational Technology (OT) vertical pack for the **Aryorithm Sentinel Ecosystem**. Designed as an Open–Closed domain extension alongside `ccsds-guard` (Aerospace), it plugs directly into `blackbox-sentinel`, `sentinel-nexus`, `sentinel-matrix`, `sentinel-lab`, and `sentinel-stack` (`aryo`) without modifying core framework code.

It combines a **zero-allocation C++20 Tier-1 deterministic protocol & safety-barrier engine** (`11.96 ns` mean / `12.87 ns` P99) with **Tier-2 `xinfer` ONNX neural inspection**, **6 `sentinel-lab` research harnesses**, a **Honeytap RK4 Mock PLC**, **3 pluggable Dual-PCAP apparatus streamers**, and a **17-container Production Master Grid digital twin**.

---

## 1. Key Capabilities & Verified Performance

| Invariant / Metric | Target / Standard | Verified Measurement |
| :--- | :--- | :--- |
| **Tier-1 Modbus TCP Inspection** | `< 120.0 ns` deterministic | **`11.9573 ns` mean / `12.867 ns` P99** (100,000 frames @ `3.18645 GHz` TSC) |
| **Runtime Heap Allocation** | `0 bytes` on hot path | Zero-allocation `std::span<const uint8_t>` + lock-free atomic air-gap |
| **eBPF/XDP Edge Mitigation** | `< 0.84 µs` line-rate drop | Active `XDP_DROP` across `Edge-Substation-01`, `Hospital-02`, `Refinery-03` |
| **Unit, ABI & ONNX Pipeline Tests** | `100%` pass | **`5/5` `ctest` suites passed** (`0.09 s`) |
| **Compliance Frameworks** | IEC 62443-4-2, EU NIS2 (2022/2555) | Live runtime compliance evaluation via `sentinel-nexus` |
| **Cloud Egress** | `$0.00` sovereign air-gapped | Local OpenVINO / TensorRT / RKNN + local web assets |

---

## 2. Two-Tier Defense Architecture

```text
  Wire Ingress (Modbus :502 | S7Comm :102 | IEC-104 :2404 | PMU :15501 | FLIR :8554 | LiDAR :2368)
         │
         ▼
  ┌─────────────────────────────────────────────────────────────────────────┐
  │ TIER 1: ics_guard_dissector_plugin.so (C++20 Native Dissector, ~12 ns)  │
  │  • Modbus TCP: MBAP bounds, illegal FC (0x08), write-lock (FC 05/06/10) │
  │  • Siemens S7Comm: TPKT/COTP + CPU STOP (0x29/0x28) + DB write block    │
  │  • IEC 60870-5-104: APCI/ASDU Type 45-47/58-64 + COT=6/8 breaker trip   │
  │  • IEEE C37.118 Synchrophasor: 49.5–50.5 Hz, |ROCOF| <= 1.0, THD <= 8%  │
  │  • Nagumo CBF Gate: h(x) = T_max - T >= 0 & dh/dt + alpha*h >= 0        │
  └──────┬───────────────────────────────────────────────────────┬──────────┘
         │ Deterministic Violation                               │ PASS (with normalized score)
         ▼                                                       ▼
   SENTINEL_VERDICT_KERNEL_DROP                       ┌───────────────────────────────┐
   + Atomic ActuatorAirGap Latch                      │ TIER 2: libxinfer.so (ONNX)   │
   (Zero ONNX Compute Overhead)                       │  • network_threat_int8.onnx   │
                                                      │  • power_quality_anomaly.onnx │
                                                      │  • vibration_spectral_ae.onnx │
                                                      │  • lidar_spatial_detector.onnx│
                                                      │  • thermal_anomaly_vision.onnx│
                                                      └───────────────────────────────┘
```

---

## 3. Repository Structure

```text
ics-guard/
├── CMakeLists.txt                       # Builds libics_guard.so/.a, dissector plugin, aryo-ics, mock_plc, tests
├── Makefile                             # init, build, test, bench, install, sync-*, grid-up/down/status/logs
├── cmake/
│   ├── IcsGuardConfig.cmake.in
│   └── ics_guard.pc.in
├── include/ics_guard/
│   ├── ics_guard.hpp                    # Umbrella C++20 header
│   ├── core/{types.hpp, tsc_clock.hpp}
│   ├── protocols/{modbus_tcp.hpp, s7comm.hpp, iec104.hpp, synchrophasor.hpp}
│   ├── defense/{plant_ode.h, cbf_safety_gate.hpp, actuator_airgap.hpp}
│   └── pipeline/guard_engine.hpp
├── src/
│   ├── core/tsc_clock.cpp
│   ├── protocols/{modbus_tcp.cpp, s7comm.cpp, iec104.cpp, synchrophasor.cpp}
│   ├── defense/{plant_ode.c, cbf_safety_gate.cpp, actuator_airgap.cpp}
│   └── pipeline/guard_engine.cpp
├── tools/
│   ├── sentinel_plugin/ics_guard_plugin.cpp  # blackbox-sentinel Tier-1 ABI dissector
│   ├── aryo_ics/main.cpp                     # CLI bridge (`aryo ics ...`)
│   ├── ics_guard_ctl/main.cpp                # Standalone diagnostic tool
│   └── mock_plc/mock_plc.c                   # Multi-protocol RK4 + Honeytap active decoy PLC
├── benchmarks/
│   └── bench_latency.cpp                     # Cycle-accurate RDTSC benchmark (< 120 ns SLA)
├── tests/
│   ├── test_modbus_validation.cpp
│   ├── test_s7_iec104_validation.cpp
│   ├── test_cbf_invariance.cpp
│   ├── test_sentinel_plugin.cpp              # dlopen() ABI verification
│   └── test_ics_tier1_tier2_pipeline.cpp     # Tier-1 C++20 vs Tier-2 xinfer OpenVINO ONNX
├── harnesses/                                # Hard-copied to /etc/sentinel/lab/harnesses & /opt/sentinel-stack/harnesses
│   ├── ics_modbus_tamper.py
│   ├── ics_iec104_trip.py
│   ├── ics_s7_runaway.py
│   ├── ics_triton_sis.py
│   ├── ics_pmu_harmonics.py
│   └── ics_airgap_tripwire.py
├── apparatus_plugins/                        # Mounted into sentinel-matrix/apparatus-runtime:latest
│   ├── synchrophasor.py
│   ├── industrial_thermal.py
│   └── reactor_core.py
├── web/                                      # Mounted to /opt/sentinel-nexus/web/plugins:ro
│   ├── manifest.json
│   └── industrial/{industrial.js, industrial.css}
└── scenarios/production_master_grid/         # 17-container OT/ICS digital twin
    ├── manifest.json
    ├── scenario.yaml
    ├── run_demo.sh
    ├── configs/{plc_register_map.json, sensor_attack_matrix.json, lab_plugins/industrial_ics.json, nodes/*.yaml}
    ├── models/                               # ONNX models + .onnx.data sidecars
    └── pcaps/                                # Industroyer2, Triton/Trisis, Stuxnet & ICS captures
```

---

## 4. Prerequisites

`ics-guard` links against the installed core Aryorithm ecosystem libraries and orchestrator:
* **`libblackbox.so`** & **`/home/kami/blackbox-sentinel/include`** (`sentinel/sdk/abi.hpp`)
* **`libxinfer.so`** (`xinfer-essential` with OpenVINO backend)
* **`sentinel-matrix`** (`/home/kami/sentinel-matrix`, with `device-runtime:latest` and `apparatus-runtime:latest`)
* **`sentinel-nexus`** (`/home/kami/sentinel-nexus`, synced binaries & domain-agnostic `web/`)
* **`sentinel-stack`** (`aryo` master CLI for `aryo ics ...` and `aryo lab ics-*` discovery)

---

## 5. Build, Test, Benchmark & Install

```bash
cd /home/kami/ics-guard

# Configure & compile release binaries (-O3 -march=native)
make build

# Run all 5 unit, ABI, and Tier-1/Tier-2 ONNX pipeline tests
make test

# Run 100,000-sample cycle-accurate latency benchmark (< 120 ns SLA)
make bench

# Install libraries, plugin, CLI, mock_plc, and hard-copy harnesses system-wide
sudo make install
```

### Installed System Artifacts
* `/usr/local/lib/libics_guard.so` & `libics_guard.a`
* `/usr/local/lib/blackbox/ics_guard_dissector_plugin.so`
* `/usr/local/bin/aryo-ics` (auto-discovered by `aryo` CLI as `aryo ics`)
* `/usr/local/bin/ics-guard-ctl`
* `/usr/local/bin/mock_plc`
* `/etc/sentinel/lab/harnesses/ics_*.py` & `/opt/sentinel-stack/harnesses/ics-*.py`

---

## 6. CLI & Research Harnesses (`aryo ics` & `aryo lab`)

### `aryo ics` Subcommands
```bash
aryo ics --help
aryo ics --plugin-desc
aryo ics bench
aryo ics verify-modbus
aryo ics verify-s7
aryo ics verify-iec104
aryo ics verify-cbf
aryo ics airgap-status
```

### `aryo lab` Industrial Research Harnesses
All 6 harnesses are hard-copied (zero symlinks) during `make init` / `sudo make install`:
```bash
aryo lab ics-modbus-tamper    # Modbus TCP FC05/06 write-lock & illegal FC08 audit
aryo lab ics-iec104-trip      # Industroyer2 IEC 60870-5-104 COT=6/8 breaker trip audit
aryo lab ics-s7-runaway       # Stuxnet S7Comm 0x29 STOP / 0x05 write & centrifuge overspin
aryo lab ics-triton-sis       # Triton/Trisis TriStation safety override & thermal spike
aryo lab ics-pmu-harmonics    # IEEE C37.118 synchrophasor frequency, ROCOF & THD audit
aryo lab ics-airgap-tripwire  # RK4 plant ODE & Nagumo CBF thermal invariance latch
```

---

## 7. Production Master Grid Digital Twin (`17` Containers on `10.240.0.0/24`)

### Launch & Operations
```bash
cd /home/kami/ics-guard

# Build, install, sync matrix/web assets, and launch all 17 containers
make grid-up        # Equivalent to: aryo ics grid-up

# Inspect live containers & port bindings
make grid-status    # Equivalent to: aryo ics grid-status

# Stream live eBPF & inference logs from Substation Alpha
make grid-logs

# Teardown mesh and release subnet/ports
make grid-down      # Equivalent to: aryo ics grid-down
```

Once live, open Mission Control at **`http://localhost:9443/`**.

### Network & Apparatus Map

| Container | IP (`10.240.0.0/24`) | Host Ports | Role / Plugin |
| :--- | :--- | :--- | :--- |
| `matrix-nexus` | `10.240.0.10` | `9443`, `9444`, `50051` | Command Plane + `/plugins/industrial/industrial.js` |
| `matrix-forge` | `10.240.0.20` | — | Continuous learning daemon (`xinfer-forge`) |
| `matrix-traffic-gen` | `10.240.0.50` | — | Synthetic ICS traffic & PCAP wave generator |
| `matrix-monitor` | `10.240.0.60` | — | Terminal real-time TUI dashboard |
| `matrix-adversary` | `10.240.0.99` | — | Live Red-Team adversary container |
| `matrix-edge-substation-01` | `10.240.0.101` | — | `Edge-Substation-01` (`INTEL_OPENVINO`, `ics_guard_dissector_plugin.so`) |
| `matrix-edge-hospital-02` | `10.240.0.102` | — | `Edge-Hospital-PACS-02` (`NVIDIA_TENSORRT`, `ics_guard_dissector_plugin.so`) |
| `matrix-edge-refinery-03` | `10.240.0.103` | — | `Edge-Refinery-PLC-03` (`ROCKCHIP_RKNN`, `ics_guard_dissector_plugin.so`) |
| `matrix-sensor-camera` | `10.240.0.75` | `8554`, `8889`, `8189/udp` | MediaMTX RTSP/WebRTC thermal camera stream |
| `matrix-sensor-vibration` | `10.240.0.76` | `15502:502` | IFM Vibration & Bearing Probe (`device-runtime`) |
| `matrix-sensor-lidar` | `10.240.0.77` | `2368/udp` | SICK Industrial LiDAR Scanner (`device-runtime`) |
| `matrix-sensor-power` | `10.240.0.78` | `15501:502` | Siemens PAC3200 Power Meter (`device-runtime`) |
| `matrix-apparatus-synchrophasor` | `10.240.0.131` | `18093:8090` | `synchrophasor.py` + `industroyer2_substation.pcap` |
| `matrix-apparatus-thermal` | `10.240.0.132` | `18094:8090` | `industrial_thermal.py` + `triton_trisis_sis.pcap` |
| `matrix-apparatus-reactor` | `10.240.0.133` | `18095:8090` | `reactor_core.py` + `stuxnet_s7comm_injected.pcap` |
| `matrix-mock-plc` | `10.240.0.150` | `502`, `102`, `2404` | Honeytap Active Decoy PLC + RK4 Plant ODE |

### Apparatus & PCAP Control Endpoints (`:18093`–`:18095`)
* `GET /health` — Plugin & container readiness check
* `GET /telemetry` — Real-time physics, harmonic THD, radiometric 16×12 grid, or reactor state
* `GET /pcaps` — Available PCAP captures mounted in `/app/pcaps`
* `POST /attack/inject` — Trigger Dual-PCAP `overlay` or `replace` burst (used by Research Lab & `Industrial SCADA C2` tab)
* `POST /attack/stop` — Immediately restore nominal baseline

---

## 8. Research & Corporate Attribution

* **Company:** Aryorithm Technologies B.V. (Amsterdam, The Netherlands)
* **Author:** Kamran Saberifard (`github.com/kamisaberi`)
* **Research Preprint:** CERN / Zenodo Permanent DOI: `10.5281/zenodo.23598779`
```
