#!/usr/bin/env python3
# File: /home/kami/ics-guard/harnesses/ics_airgap_tripwire.py

"""
Sentinel-Lab: Nagumo Control Barrier Function (CBF) Actuator Air-Gap Latch Audit
"""

import argparse


class IcsAirgapTripwireHarness:
    def evaluate_cbf(self, temp_c: float, heater_power: float, valve_open: float, max_temp: float = 120.0, alpha: float = 0.5):
        h = max_temp - temp_c
        dT_dt = (5.0 * heater_power) - (0.05 * (temp_c - 65.0)) - (0.1 * valve_open)
        h_dot = -dT_dt
        invariance = h_dot + alpha * h
        return invariance >= 0.0 and h >= 0.0, h, invariance

    def run_audit(self, mode="runaway"):
        print("\033[1;36m================================================================================\033[0m")
        print("\033[1;37m   SENTINEL-LAB: NAGUMO CBF ACTUATOR AIR-GAP INVARIANCE AUDIT                   \033[0m")
        print("\033[1;36m================================================================================\033[0m")
        cases = [
            ("Nominal Circulation", 85.4, 0.2, 0.1),
            ("Exothermic Runaway", 134.2, 1.0, 0.0) if mode == "runaway" else ("Boundary Heating Surge", 118.5, 1.0, 0.0),
        ]
        for label, t, hp, vo in cases:
            ok, h, inv = self.evaluate_cbf(t, hp, vo)
            col = "\033[1;32mPASS\033[0m" if ok else "\033[1;31mAIRGAP_LATCHED\033[0m"
            print(f"[*] {label:<26} | T={t:.1f}C | h(x)={h:+.2f} | dh/dt+ah={inv:+.2f} -> {col}")
        print("\033[1;36m================================================================================\033[0m\n")


def main():
    parser = argparse.ArgumentParser(description="ICS Nagumo CBF Air-Gap Audit")
    parser.add_argument("--mode", default="runaway", choices=["runaway", "boundary"])
    args = parser.parse_args()
    IcsAirgapTripwireHarness().run_audit(mode=args.mode)


if __name__ == "__main__":
    main()
