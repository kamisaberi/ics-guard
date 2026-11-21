#!/usr/bin/env python3
# File: /home/kami/ics-guard/harnesses/ics_triton_sis.py

"""
Sentinel-Lab: Safety Instrumented System (SIS) TriStation & Thermal Runaway Audit (TRITON/Trisis)
"""

import argparse


class IcsTritonSisHarness:
    def evaluate_sis(self, peak_temp_c: float, saturation_pct: float, sis_program_write: bool):
        if sis_program_write:
            return False, "REJECT_SIS_PROGRAM_OVERRIDE"
        if peak_temp_c > 120.0 or saturation_pct > 55.0:
            return False, "REJECT_THERMAL_RUNAWAY_AIRGAP"
        return True, "PASS"

    def run_audit(self, mode="triton"):
        print("\033[1;36m================================================================================\033[0m")
        print("\033[1;37m   SENTINEL-LAB: TRITON / TRISIS SIS SAFETY OVERRIDE AUDIT                      \033[0m")
        print("\033[1;36m================================================================================\033[0m")
        cases = [
            ("Nominal Hospital Cooling ROI", 38.5, 4.2, False),
            ("TriStation Safety Logic Download" if mode == "triton" else "Radiometric Runaway Spike",
             146.5 if mode != "triton" else 89.0, 94.8 if mode != "triton" else 12.0, mode == "triton"),
        ]
        for label, t, sat, wr in cases:
            ok, verdict = self.evaluate_sis(t, sat, wr)
            col = "\033[1;32m" if ok else "\033[1;31m"
            print(f"[*] {label:<38} | T={t:.1f}C SAT={sat:.1f}% | {col}{verdict}\033[0m")
        print("\033[1;36m================================================================================\033[0m\n")


def main():
    parser = argparse.ArgumentParser(description="ICS Triton/Trisis SIS Audit")
    parser.add_argument("--mode", default="triton", choices=["triton", "thermal_spike"])
    args = parser.parse_args()
    IcsTritonSisHarness().run_audit(mode=args.mode)


if __name__ == "__main__":
    main()
