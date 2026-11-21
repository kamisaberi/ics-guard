#!/usr/bin/env python3
# File: /home/kami/ics-guard/harnesses/ics_s7_runaway.py

"""
Sentinel-Lab: Siemens S7Comm Centrifuge Frequency Tamper & RK4 Thermal Runaway Audit (Stuxnet)
"""

import argparse


class IcsS7RunawayHarness:
    def evaluate_s7(self, func: int, rotor_rpm: float, max_rpm: float = 2200.0):
        if func in (0x28, 0x29):
            return False, "REJECT_S7_CPU_STOP"
        if func in (0x05, 0x1A, 0x1B, 0x1C):
            return False, "REJECT_S7_UNAUTHORIZED_WRITE"
        if rotor_rpm > max_rpm:
            return False, "REJECT_CBF_CENTRIFUGE_OVERSPIN"
        return True, "PASS"

    def run_audit(self, mode="stuxnet"):
        print("\033[1;36m================================================================================\033[0m")
        print("\033[1;37m   SENTINEL-LAB: S7COMM & CENTRIFUGE OVERSPIN AUDIT (STUXNET VECTOR)            \033[0m")
        print("\033[1;36m================================================================================\033[0m")
        steps = [
            ("Nominal S7 Read Var (0x04)", 0x04, 1450.0),
            ("Stuxnet DB Write (0x05) + 3600 RPM" if mode == "stuxnet" else "S7 CPU STOP (0x29)",
             0x05 if mode == "stuxnet" else 0x29, 3600.0 if mode == "stuxnet" else 0.0),
        ]
        for label, fn, rpm in steps:
            ok, verdict = self.evaluate_s7(fn, rpm)
            col = "\033[1;32m" if ok else "\033[1;31m"
            print(f"[*] {label:<40} | RPM={rpm:<6.1f} | {col}{verdict}\033[0m")
        print("\033[1;36m================================================================================\033[0m\n")


def main():
    parser = argparse.ArgumentParser(description="ICS S7Comm & Centrifuge Runaway Audit")
    parser.add_argument("--mode", default="stuxnet", choices=["stuxnet", "cpu_stop"])
    args = parser.parse_args()
    IcsS7RunawayHarness().run_audit(mode=args.mode)


if __name__ == "__main__":
    main()
