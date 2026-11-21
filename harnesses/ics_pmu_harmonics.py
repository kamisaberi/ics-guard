#!/usr/bin/env python3
# File: /home/kami/ics-guard/harnesses/ics_pmu_harmonics.py

"""
Sentinel-Lab: IEEE C37.118 Synchrophasor Frequency, ROCOF & Harmonic THD Audit
"""

import argparse


class IcsPmuHarmonicsHarness:
    def evaluate_pmu(self, freq_hz: float, rocof: float, thd_pct: float):
        if freq_hz < 49.50 or freq_hz > 50.50:
            return False, "REJECT_PMU_FREQ_EXCURSION"
        if abs(rocof) > 1.00:
            return False, "REJECT_PMU_ROCOF_BREACH"
        if thd_pct > 8.00:
            return False, "REJECT_PMU_HARMONIC_THD"
        return True, "PASS"

    def run_audit(self, mode="harmonic"):
        print("\033[1;36m================================================================================\033[0m")
        print("\033[1;37m   SENTINEL-LAB: IEEE C37.118 SYNCHROPHASOR & HARMONICS AUDIT                   \033[0m")
        print("\033[1;36m================================================================================\033[0m")
        cases = [
            ("Nominal 50Hz Grid Busbar", 50.01, 0.02, 1.4),
            ("Harmonic Resonance (THD=21.8%)" if mode == "harmonic" else "Underfrequency ROCOF Trip",
             49.42 if mode != "harmonic" else 49.82, 1.45 if mode != "harmonic" else 0.35, 21.8 if mode == "harmonic" else 4.2),
        ]
        for label, f, r, thd in cases:
            ok, verdict = self.evaluate_pmu(f, r, thd)
            col = "\033[1;32m" if ok else "\033[1;31m"
            print(f"[*] {label:<34} | f={f:.2f}Hz ROCOF={r:.2f} THD={thd:.1f}% | {col}{verdict}\033[0m")
        print("\033[1;36m================================================================================\033[0m\n")


def main():
    parser = argparse.ArgumentParser(description="ICS IEEE C37.118 Synchrophasor Audit")
    parser.add_argument("--mode", default="harmonic", choices=["harmonic", "rocof"])
    args = parser.parse_args()
    IcsPmuHarmonicsHarness().run_audit(mode=args.mode)


if __name__ == "__main__":
    main()
