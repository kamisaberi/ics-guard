# File: /home/kami/ics-guard/apparatus_plugins/reactor_core.py

import math
import struct
import time


class ReactorCorePlugin:
    def init_state(self, engine):
        engine.domain_state["reactor"] = {
            "pressure_psi": 100.2,
            "temp_c": 85.4,
            "flow_lpm": 42.0,
            "agitator_rpm": 1200,
            "quench_active": False,
            "status": "NOMINAL_CIRCULATION",
        }

    def decode_packet(self, engine, payload: bytes, is_attack: bool, src_ip: str = ""):
        if is_attack:
            rx = engine.domain_state["reactor"]
            rx["pressure_psi"] = round(168.5 + math.sin(time.time() * 6.0) * 4.2, 1)
            rx["temp_c"] = round(134.2 + math.sin(time.time() * 4.0) * 2.5, 1)
            rx["agitator_rpm"] = 3600
            rx["quench_active"] = False
            rx["status"] = "EXOTHERMIC_RUNAWAY_DANGER"

    def emit_nominal_step(self, engine, elapsed: float):
        tick = elapsed * 0.55
        with engine.lock:
            if not engine.active_attack["active"]:
                rx = engine.domain_state["reactor"]
                rx["pressure_psi"] = round(100.2 + math.sin(tick * 1.4) * 1.5, 1)
                rx["temp_c"] = round(85.4 + math.sin(tick * 0.9) * 0.8, 1)
                rx["flow_lpm"] = round(42.0 + math.cos(tick * 1.1) * 0.5, 1)
                rx["agitator_rpm"] = int(1200 + math.sin(tick * 2.0) * 15)
                rx["quench_active"] = False
                rx["status"] = "NOMINAL_CIRCULATION"
            p = engine.domain_state["reactor"]["pressure_psi"]
        pkt = struct.pack(">4sf", b"S7RX", float(p))
        engine.send_and_decode(pkt, is_attack=False)

    def emit_attack_step(self, engine, attack_type: str):
        pkt = struct.pack(">4sf", b"S7!!", 168.5)
        engine.send_and_decode(pkt, is_attack=True)

    def on_attack_triggered(self, engine, attack_type: str):
        rx = engine.domain_state["reactor"]
        rx["pressure_psi"] = 168.5
        rx["temp_c"] = 134.2
        rx["agitator_rpm"] = 3600
        rx["status"] = "EXOTHERMIC_RUNAWAY_DANGER"

    def on_attack_stopped(self, engine):
        self.init_state(engine)

    def handle_post(self, engine, path: str, req: dict):
        return None

    def get_snapshot(self, engine) -> dict:
        return {"reactor": dict(engine.domain_state["reactor"])}


APPARATUS_TYPES = {"reactor_core": ReactorCorePlugin}
