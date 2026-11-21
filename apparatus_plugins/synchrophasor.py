# File: /home/kami/ics-guard/apparatus_plugins/synchrophasor.py

import math
import struct
import time


class SynchrophasorPlugin:
    def init_state(self, engine):
        engine.domain_state["synchrophasor"] = {
            "freq_hz": 50.00,
            "busbar_kv": 220.4,
            "phase_deg": 0.0,
            "thd_pct": 1.4,
            "harmonics": {"1x": 1.0, "2x": 0.12, "3x": 0.08, "5x": 0.04},
        }

    def decode_packet(self, engine, payload: bytes, is_attack: bool, src_ip: str = ""):
        if is_attack:
            s = engine.domain_state["synchrophasor"]
            s["freq_hz"] = round(49.42 + math.sin(time.time() * 8.0) * 0.18, 3)
            s["thd_pct"] = 21.8
            s["harmonics"] = {"1x": 1.0, "2x": 0.64, "3x": 0.52, "5x": 0.31}

    def emit_nominal_step(self, engine, elapsed: float):
        tick = elapsed * 0.55
        with engine.lock:
            if not engine.active_attack["active"]:
                s = engine.domain_state["synchrophasor"]
                s["freq_hz"] = round(50.00 + math.sin(tick * 1.5) * 0.025, 3)
                s["phase_deg"] = round((elapsed * 45.0) % 360.0, 1)
                s["thd_pct"] = 1.4
                s["harmonics"] = {"1x": 1.0, "2x": 0.12, "3x": 0.08, "5x": 0.04}
                freq = s["freq_hz"]
            else:
                freq = engine.domain_state["synchrophasor"]["freq_hz"]
        pkt = struct.pack(">4sf", b"PMU1", float(freq))
        engine.send_and_decode(pkt, is_attack=False)

    def emit_attack_step(self, engine, attack_type: str):
        pkt = struct.pack(">4sf", b"PMU!", 49.42)
        engine.send_and_decode(pkt, is_attack=True)

    def on_attack_triggered(self, engine, attack_type: str):
        s = engine.domain_state["synchrophasor"]
        s["freq_hz"] = 49.42
        s["thd_pct"] = 21.8
        s["harmonics"] = {"1x": 1.0, "2x": 0.64, "3x": 0.52, "5x": 0.31}

    def on_attack_stopped(self, engine):
        self.init_state(engine)

    def handle_post(self, engine, path: str, req: dict):
        return None

    def get_snapshot(self, engine) -> dict:
        return {"synchrophasor": dict(engine.domain_state["synchrophasor"])}


APPARATUS_TYPES = {"synchrophasor": SynchrophasorPlugin}
