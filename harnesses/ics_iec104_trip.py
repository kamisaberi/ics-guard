#!/usr/bin/env python3
# File: /home/kami/ics-guard/harnesses/ics_iec104_trip.py

"""
Sentinel-Lab: IEC 60870-5-104 Substation Breaker Trip Audit (Industroyer2)
"""

import argparse


class IcsIec104TripHarness:
    TELECONTROL_TYPES = {45, 46, 47, 58, 59, 60, 61, 62, 63, 64}

    def evaluate_asdu(self, frame_type: str, type_id: int, cot: int):
        if frame_type in ("U", "S"):
            return True, "PASS"
        if type_id in self.TELECONTROL_TYPES and cot in (6, 8):
            return False, "REJECT_IEC104_SPURIOUS_TRIP"
        return True, "PASS"

    def run_audit(self, mode="industroyer2"):
        print("\033[1;36m================================================================================\033[0m")
        print("\033[1;37m   SENTINEL-LAB: IEC 60870-5-104 BREAKER TRIP AUDIT (INDUSTROYER2)              \033[0m")
        print("\033[1;36m================================================================================\033[0m")
        cases = [
            ("APCI TESTFR U-Frame", "U", 0, 0),
            ("Spontaneous Measurand (Type 13, COT=3)", "I", 13, 3),
            ("Breaker Trip Single Cmd (Type 45, COT=6)" if mode == "industroyer2" else "Double Cmd Trip (Type 46, COT=8)",
             "I", 45 if mode == "industroyer2" else 46, 6 if mode == "industroyer2" else 8),
        ]
        for label, ft, tid, cot in cases:
            ok, verdict = self.evaluate_asdu(ft, tid, cot)
            col = "\033[1;32m" if ok else "\033[1;31m"
            print(f"[*] {label:<44} | {col}{verdict}\033[0m")
        print("\033[1;36m================================================================================\033[0m\n")


def main():
    parser = argparse.ArgumentParser(description="ICS IEC-104 Breaker Trip Audit")
    parser.add_argument("--mode", default="industroyer2", choices=["industroyer2", "double_cmd"])
    args = parser.parse_args()
    IcsIec104TripHarness().run_audit(mode=args.mode)


if __name__ == "__main__":
    main()
