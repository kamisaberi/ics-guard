// File: /home/kami/ics-guard/web/industrial/industrial.js
// ==============================================================================
// ARYORITHM ICS-GUARD UI PLUGIN:
//   1. Substation Power Quality & Synchrophasor Scope (:18093)
//   2. Hospital / Facility Radiometric Thermal Vision Camera (:18094)
//   3. Refinery Exothermic Reaction Vessel Core (:18095)
//   4. Industrial SCADA & Critical Infrastructure C2 Tab
// ==============================================================================

(function () {
    if (!window.SentinelUI) return;

    const HOST = window.location.hostname || "127.0.0.1";
    const SYNCHRO_API = `http://${HOST}:18093`;
    const THERMAL_API = `http://${HOST}:18094`;
    const REACTOR_API = `http://${HOST}:18095`;

    const isAerospace = (node) =>
        /aerospace|uav|swarm/i.test(node?.site || "") || /drone/i.test(node?.hostname || "");

    const isIndustrialNode = (node) => !isAerospace(node);

    // 1. Live Radiometric Thermal Camera (:18094)
    window.SentinelUI.registerApparatus({
        id: "industrial_radiometric_vision",
        title: "RADIOMETRIC THERMAL VISION CAMERA",
        shortLabel: "📷 THERMAL CAM",
        aspectRatio: 1.25,
        protocol: "WHEP / RTSP :8554",
        defaultForTypes: ["video"],
        matches: (node) =>
            isIndustrialNode(node) &&
            (node?.primary_instrument?.type || "").toLowerCase() === "video",
        mount: function (container) {
            container.innerHTML = `<div style="position:relative; width:100%; height:100%; background:#060418; border-radius:4px; overflow:hidden; aspect-ratio:1.25;"><canvas width="750" height="600" style="width:100%; height:100%; object-fit:cover; display:block;"></canvas></div>`;
            const canvas = container.querySelector("canvas");
            const ctx = canvas.getContext("2d");
            const state = { animId: null, alive: true, data: null };

            const poll = async () => {
                try {
                    const r = await fetch(`${THERMAL_API}/telemetry`, { cache: "no-store" });
                    if (r.ok) state.data = await r.json();
                } catch (_) { }
            };
            poll();
            const timer = setInterval(poll, 250);

            function loop() {
                if (!state.alive) return;
                const w = canvas.width, h = canvas.height;
                const flir = state.data?.flir;
                const isDazzle = Boolean(flir?.laser_dazzle || state.data?.active_attack?.active);
                ctx.fillStyle = "#060418";
                ctx.fillRect(0, 0, w, h);

                const grid = flir?.thermal_grid_c || [];
                let maxTemp = -999, hotR = 6, hotC = 8;
                if (grid.length > 0 && grid[0].length > 0) {
                    const rows = grid.length, cols = grid[0].length;
                    const cellW = w / cols, cellH = h / rows;
                    for (let r = 0; r < rows; r++) {
                        for (let c = 0; c < cols; c++) {
                            const temp = grid[r][c];
                            if (temp > maxTemp) { maxTemp = temp; hotR = r; hotC = c; }
                            const t = Math.max(0, Math.min(1, (temp - 20) / 40));
                            ctx.fillStyle = isDazzle && temp > 100
                                ? "#FFFFFF"
                                : `rgb(${Math.round(t * 255)}, ${Math.round(40 + t * 160)}, ${Math.round(180 - t * 120)})`;
                            ctx.fillRect(c * cellW, r * cellH, Math.ceil(cellW + 1), Math.ceil(cellH + 1));
                        }
                    }
                    const hx = ((hotC + 0.5) / cols) * w;
                    const hy = ((hotR + 0.5) / rows) * h;
                    ctx.strokeStyle = isDazzle ? "#FF3366" : "#00FFA3";
                    ctx.lineWidth = 1.6;
                    ctx.strokeRect(hx - 22, hy - 22, 44, 44);
                }

                ctx.fillStyle = "rgba(7, 11, 20, 0.85)";
                ctx.fillRect(0, 0, w, 28);
                ctx.fillRect(0, h - 26, w, 26);
                ctx.font = "bold 10.5px monospace";
                ctx.fillStyle = isDazzle ? "#FF3366" : "#00FFA3";
                ctx.fillText(
                    isDazzle
                        ? `[!] THERMAL RUNAWAY / TRITON OVERRIDE | ${Number(flir?.peak_temp_c || 146.5).toFixed(1)}°C`
                        : `[+] RADIOMETRIC FLIR | PEAK: ${Number(flir?.peak_temp_c || 38.5).toFixed(1)}°C | AMBIENT: 21.4°C`,
                    12, 18
                );
                ctx.font = "9px monospace";
                ctx.fillStyle = "#94A3B8";
                ctx.fillText("STREAM: WHEP / RTSP :8554 | ROI TARGET: HOSPITAL_PACS_COOLING [LOCKED]", 12, h - 9);
                state.animId = requestAnimationFrame(loop);
            }
            loop();
            state.cleanup = () => clearInterval(timer);
            return state;
        },
        unmount: function (handle) {
            if (!handle) return;
            handle.alive = false;
            if (handle.animId) cancelAnimationFrame(handle.animId);
            if (handle.cleanup) handle.cleanup();
        },
        getPcapConfig: () => ({
            apiBase: THERMAL_API,
            attacks: [
                { label: "[!] Replay Triton TriStation PCAP (Overlay)", pcap: "triton_trisis_sis.pcap", attack_type: "triton", mode: "overlay", duration_sec: 15 }
            ]
        })
    });

    // 2. Live Synchrophasor Scope (:18093)
    window.SentinelUI.registerApparatus({
        id: "industrial_synchrophasor_scope",
        title: "POWER QUALITY & SYNCHROPHASOR SCOPE",
        shortLabel: "⚡ GRID SCOPE",
        aspectRatio: 1.25,
        protocol: "IEEE C37.118 / MODBUS :15501",
        defaultForTypes: ["grid"],
        matches: (node) =>
            isIndustrialNode(node) &&
            ((node?.primary_instrument?.type || "").toLowerCase() === "grid" ||
             /substation/i.test(node?.hostname || "")),
        mount: function (container) {
            container.innerHTML = `<div style="position:relative; width:100%; height:100%; background:#05080C; border-radius:4px; overflow:hidden; aspect-ratio:1.25;"><canvas width="750" height="600" style="width:100%; height:100%; object-fit:cover; display:block;"></canvas></div>`;
            const canvas = container.querySelector("canvas");
            const ctx = canvas.getContext("2d");
            const state = { animId: null, alive: true, data: null, tick: 0 };

            const poll = async () => {
                try {
                    const r = await fetch(`${SYNCHRO_API}/telemetry`, { cache: "no-store" });
                    if (r.ok) state.data = await r.json();
                } catch (_) { }
            };
            poll();
            const timer = setInterval(poll, 250);

            function loop() {
                if (!state.alive) return;
                state.tick += 0.04;
                const w = canvas.width, h = canvas.height;
                const syn = state.data?.synchrophasor;
                const isDistorted = Boolean(state.data?.active_attack?.active || (syn?.thd_pct || 0) > 10);

                ctx.fillStyle = "#05080C";
                ctx.fillRect(0, 0, w, h);

                // Grid reference lines
                ctx.strokeStyle = "rgba(0, 229, 255, 0.12)";
                ctx.lineWidth = 1;
                ctx.beginPath();
                ctx.moveTo(0, h / 2); ctx.lineTo(w, h / 2);
                ctx.moveTo(w / 2, 30); ctx.lineTo(w / 2, h - 28);
                ctx.stroke();

                // Fundamental + 3rd/5th Harmonic Waveform
                ctx.strokeStyle = isDistorted ? "#FF3366" : "#00F0FF";
                ctx.lineWidth = 2.4;
                ctx.beginPath();
                for (let x = 0; x < w; x++) {
                    const base = Math.sin((x * 0.015) + (state.tick * 3.5)) * 75;
                    const harm = isDistorted
                        ? Math.sin((x * 0.045) + (state.tick * 8.0)) * 32 + Math.sin((x * 0.075) - (state.tick * 11.0)) * 18
                        : Math.sin((x * 0.045) + (state.tick * 3.5)) * 3;
                    const y = (h / 2) + base + harm;
                    if (x === 0) ctx.moveTo(x, y); else ctx.lineTo(x, y);
                }
                ctx.stroke();

                ctx.fillStyle = "rgba(7, 11, 20, 0.85)";
                ctx.fillRect(0, 0, w, 28);
                ctx.fillRect(0, h - 26, w, 26);
                ctx.font = "bold 10.5px monospace";
                ctx.fillStyle = isDistorted ? "#FF3366" : "#00E5FF";
                ctx.fillText(
                    isDistorted
                        ? `[!] GRID HARMONIC DISTORTION INJECTED | FREQ: ${syn?.freq_hz || 49.42} Hz | THD: ${syn?.thd_pct || 21.8}%`
                        : `[+] SYNCHROPHASOR SCOPE | FREQ: ${syn?.freq_hz || 50.00} Hz | THD: ${syn?.thd_pct || 1.4}%`,
                    12, 18
                );
                ctx.font = "9px monospace";
                ctx.fillStyle = "#94A3B8";
                ctx.fillText(
                    `BUSBAR: ${syn?.busbar_kv || 220.4} kV  |  PHASE: ${syn?.phase_deg || 0}°  |  IEC-104 + MODBUS SLA: < 120 ns DROP`,
                    12, h - 9
                );
                state.animId = requestAnimationFrame(loop);
            }
            loop();
            state.cleanup = () => clearInterval(timer);
            return state;
        },
        unmount: function (handle) {
            if (!handle) return;
            handle.alive = false;
            if (handle.animId) cancelAnimationFrame(handle.animId);
            if (handle.cleanup) handle.cleanup();
        },
        getPcapConfig: () => ({
            apiBase: SYNCHRO_API,
            attacks: [
                { label: "[!] Replay Industroyer2 IEC-104 PCAP (Overlay)", pcap: "industroyer2_substation.pcap", attack_type: "industroyer", mode: "overlay", duration_sec: 15 }
            ]
        })
    });

    // 3. Live Exothermic Reaction Vessel (:18095)
    window.SentinelUI.registerApparatus({
        id: "industrial_exothermic_reactor",
        title: "EXOTHERMIC REACTION VESSEL CORE",
        shortLabel: "☢️ REACTOR CORE",
        aspectRatio: 1.25,
        protocol: "S7COMM :102 / HART-IP",
        defaultForTypes: ["reactor"],
        matches: (node) =>
            isIndustrialNode(node) &&
            ((node?.primary_instrument?.type || "").toLowerCase() === "reactor" ||
             /refinery/i.test(node?.hostname || "")),
        mount: function (container) {
            container.innerHTML = `<div style="position:relative; width:100%; height:100%; background:#05080C; border-radius:4px; overflow:hidden; aspect-ratio:1.25;"><canvas width="750" height="600" style="width:100%; height:100%; object-fit:cover; display:block;"></canvas></div>`;
            const canvas = container.querySelector("canvas");
            const ctx = canvas.getContext("2d");
            const state = { animId: null, alive: true, data: null, tick: 0 };

            const poll = async () => {
                try {
                    const r = await fetch(`${REACTOR_API}/telemetry`, { cache: "no-store" });
                    if (r.ok) state.data = await r.json();
                } catch (_) { }
            };
            poll();
            const timer = setInterval(poll, 250);

            function loop() {
                if (!state.alive) return;
                const rx = state.data?.reactor || { pressure_psi: 100.2, temp_c: 85.4, agitator_rpm: 1200 };
                const isRunaway = Boolean(state.data?.active_attack?.active || rx.pressure_psi > 140);
                state.tick += isRunaway ? 0.14 : 0.04;

                const w = canvas.width, h = canvas.height;
                const cx = w / 2, cy = h / 2;

                const grad = ctx.createRadialGradient(cx, cy, 18, cx, cy, w / 1.5);
                grad.addColorStop(0, isRunaway ? "rgba(255, 51, 102, 0.72)" : "rgba(168, 85, 247, 0.48)");
                grad.addColorStop(1, "#05080C");
                ctx.fillStyle = grad;
                ctx.fillRect(0, 0, w, h);

                // Vessel Core Rings & Agitator Rotor
                ctx.strokeStyle = isRunaway ? "#FF3366" : "#A855F7";
                ctx.lineWidth = 2.2;
                ctx.beginPath();
                ctx.arc(cx, cy, 125, 0, Math.PI * 2);
                ctx.stroke();

                ctx.save();
                ctx.translate(cx, cy);
                ctx.rotate(state.tick);
                ctx.strokeStyle = isRunaway ? "#FF99AA" : "#00FFA3";
                ctx.lineWidth = 3;
                for (let b = 0; b < 4; b++) {
                    ctx.rotate(Math.PI / 2);
                    ctx.beginPath();
                    ctx.moveTo(0, 0);
                    ctx.lineTo(92, 0);
                    ctx.stroke();
                }
                ctx.restore();

                ctx.fillStyle = "rgba(7, 11, 20, 0.85)";
                ctx.fillRect(0, 0, w, 28);
                ctx.fillRect(0, h - 26, w, 26);
                ctx.font = "bold 10.5px monospace";
                ctx.fillStyle = isRunaway ? "#FF3366" : "#A855F7";
                ctx.fillText(
                    isRunaway
                        ? `[!] EXOTHERMIC REACTION RUNAWAY | ${rx.pressure_psi} PSI | CBF AIRGAP LATCHED`
                        : `[+] EXOTHERMIC REACTION VESSEL CORE | ${rx.pressure_psi} PSI | S7COMM :102`,
                    12, 18
                );
                ctx.font = "9px monospace";
                ctx.fillStyle = "#94A3B8";
                ctx.fillText(
                    `CORE TEMP: ${rx.temp_c}°C  |  AGITATOR: ${rx.agitator_rpm} RPM  |  NAGUMO CBF NITROGEN QUENCH: ARMED`,
                    12, h - 9
                );
                state.animId = requestAnimationFrame(loop);
            }
            loop();
            state.cleanup = () => clearInterval(timer);
            return state;
        },
        unmount: function (handle) {
            if (!handle) return;
            handle.alive = false;
            if (handle.animId) cancelAnimationFrame(handle.animId);
            if (handle.cleanup) handle.cleanup();
        },
        getPcapConfig: () => ({
            apiBase: REACTOR_API,
            attacks: [
                { label: "[!] Replay Stuxnet S7Comm PCAP (Overlay)", pcap: "stuxnet_s7comm_injected.pcap", attack_type: "stuxnet", mode: "overlay", duration_sec: 15 }
            ]
        })
    });

    // 4. Dedicated Industrial SCADA Operations C2 Tab
    let c2PollTimer = null;
    function renderIcsC2(container) {
        if (c2PollTimer) clearInterval(c2PollTimer);
        container.innerHTML = `
            <div style="display:flex; flex-direction:column; gap:14px;">
                <div class="ics-c2-bar">
                    <div style="display:flex; align-items:center; gap:10px;">
                        <span style="font-size:12px; font-weight:800; color:#00E5FF; letter-spacing:0.08em;">
                            ICS-GUARD CRITICAL INFRASTRUCTURE &amp; SCADA C2
                        </span>
                        <span class="badge badge-green" style="font-size:9px;">SUB-120 ns TIER-1 TRIPWIRE</span>
                        <span class="badge badge-cyan" style="font-size:9px;">IEC 62443 / EU NIS2</span>
                    </div>
                    <div style="font-family:monospace; font-size:11px; color:#94A3B8;">
                        LIVE APPARATUS STREAMS: :18093 (PMU) &bull; :18094 (FLIR) &bull; :18095 (REACTOR)
                    </div>
                </div>

                <div class="ics-c2-grid">
                    <div id="ics-card-pmu" class="ics-c2-card">
                        <div style="color:#00E5FF; font-weight:800; font-size:11.5px;">⚡ SUBSTATION ALPHA (10.240.0.101)</div>
                        <div id="ics-pmu-kpis">Connecting to :18093...</div>
                        <div style="display:flex; gap:8px; margin-top:6px;">
                            <button class="ics-c2-btn danger" data-ics-atk="pmu">INJECT INDUSTROYER2</button>
                            <button class="ics-c2-btn" data-ics-stop="pmu">RESTORE</button>
                        </div>
                    </div>

                    <div id="ics-card-flir" class="ics-c2-card">
                        <div style="color:#00FFA3; font-weight:800; font-size:11.5px;">📷 HOSPITAL PACS / FLIR (10.240.0.102)</div>
                        <div id="ics-flir-kpis">Connecting to :18094...</div>
                        <div style="display:flex; gap:8px; margin-top:6px;">
                            <button class="ics-c2-btn danger" data-ics-atk="flir">INJECT TRITON SIS</button>
                            <button class="ics-c2-btn" data-ics-stop="flir">RESTORE</button>
                        </div>
                    </div>

                    <div id="ics-card-rx" class="ics-c2-card">
                        <div style="color:#A855F7; font-weight:800; font-size:11.5px;">☢️ REFINERY REACTOR (10.240.0.103)</div>
                        <div id="ics-rx-kpis">Connecting to :18095...</div>
                        <div style="display:flex; gap:8px; margin-top:6px;">
                            <button class="ics-c2-btn danger" data-ics-atk="rx">INJECT STUXNET S7</button>
                            <button class="ics-c2-btn" data-ics-stop="rx">RESTORE</button>
                        </div>
                    </div>
                </div>
            </div>
        `;

        const endpoints = {
            pmu: { url: SYNCHRO_API, payload: { attack_type: "industroyer", pcap: "industroyer2_substation.pcap", mode: "overlay", duration_sec: 15 } },
            flir: { url: THERMAL_API, payload: { attack_type: "triton", pcap: "triton_trisis_sis.pcap", mode: "overlay", duration_sec: 15 } },
            rx: { url: REACTOR_API, payload: { attack_type: "stuxnet", pcap: "stuxnet_s7comm_injected.pcap", mode: "overlay", duration_sec: 15 } }
        };

        container.querySelectorAll("[data-ics-atk]").forEach(btn => {
            btn.addEventListener("click", () => {
                const key = btn.getAttribute("data-ics-atk");
                const cfg = endpoints[key];
                if (!cfg) return;
                fetch(`${cfg.url}/attack/inject`, {
                    method: "POST",
                    headers: { "Content-Type": "application/json" },
                    body: JSON.stringify(cfg.payload)
                }).catch(() => {});
            });
        });

        container.querySelectorAll("[data-ics-stop]").forEach(btn => {
            btn.addEventListener("click", () => {
                const key = btn.getAttribute("data-ics-stop");
                const cfg = endpoints[key];
                if (!cfg) return;
                fetch(`${cfg.url}/attack/stop`, { method: "POST" }).catch(() => {});
            });
        });

        async function refresh() {
            const pmu = await fetch(`${SYNCHRO_API}/telemetry`, { cache: "no-store" }).then(r => r.ok ? r.json() : null).catch(() => null);
            const flir = await fetch(`${THERMAL_API}/telemetry`, { cache: "no-store" }).then(r => r.ok ? r.json() : null).catch(() => null);
            const rx = await fetch(`${REACTOR_API}/telemetry`, { cache: "no-store" }).then(r => r.ok ? r.json() : null).catch(() => null);

            if (pmu) {
                const s = pmu.synchrophasor || {};
                const alert = Boolean(pmu.active_attack?.active || (s.thd_pct || 0) > 10);
                document.getElementById("ics-card-pmu")?.classList.toggle("alert", alert);
                const el = document.getElementById("ics-pmu-kpis");
                if (el) el.innerHTML = `
                    <div class="ics-c2-kpi"><span>Grid Frequency</span><b>${s.freq_hz ?? 50.0} Hz</b></div>
                    <div class="ics-c2-kpi"><span>Harmonic THD</span><b style="color:${alert ? "#FF3366" : "#00FFA3"};">${s.thd_pct ?? 1.4}%</b></div>
                    <div class="ics-c2-kpi"><span>Busbar Voltage</span><b>${s.busbar_kv ?? 220.4} kV</b></div>`;
            }
            if (flir) {
                const f = flir.flir || {};
                const alert = Boolean(flir.active_attack?.active || f.laser_dazzle);
                document.getElementById("ics-card-flir")?.classList.toggle("alert", alert);
                const el = document.getElementById("ics-flir-kpis");
                if (el) el.innerHTML = `
                    <div class="ics-c2-kpi"><span>Peak Thermal ROI</span><b style="color:${alert ? "#FF3366" : "#00FFA3"};">${f.peak_temp_c ?? 38.5} °C</b></div>
                    <div class="ics-c2-kpi"><span>Sensor Saturation</span><b>${f.saturation_pct ?? 4.2}%</b></div>
                    <div class="ics-c2-kpi"><span>Slant Range</span><b>${f.lrf_slant_range_m ?? 412} m</b></div>`;
            }
            if (rx) {
                const r = rx.reactor || {};
                const alert = Boolean(rx.active_attack?.active || (r.pressure_psi || 0) > 140);
                document.getElementById("ics-card-rx")?.classList.toggle("alert", alert);
                const el = document.getElementById("ics-rx-kpis");
                if (el) el.innerHTML = `
                    <div class="ics-c2-kpi"><span>Vessel Pressure</span><b style="color:${alert ? "#FF3366" : "#00FFA3"};">${r.pressure_psi ?? 100.2} PSI</b></div>
                    <div class="ics-c2-kpi"><span>Core Temp</span><b>${r.temp_c ?? 85.4} °C</b></div>
                    <div class="ics-c2-kpi"><span>Agitator RPM</span><b>${r.agitator_rpm ?? 1200} RPM</b></div>`;
            }
        }

        refresh();
        c2PollTimer = setInterval(refresh, 500);
    }

    window.SentinelUI.registerTab({
        id: "tab-ics-c2",
        label: "Industrial SCADA C2",
        icon: "🏭",
        group: "Operations",
        renderView: renderIcsC2
    });

    console.log("[ICS-Guard Plugin] Industrial Synchrophasor, Thermal, Reactor & SCADA C2 registered.");
})();
