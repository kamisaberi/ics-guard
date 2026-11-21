#!/usr/bin/env python3
# File: /home/kami/ics-guard/harnesses/ics_modbus_tamper.py

"""
Sentinel-Lab: Modbus TCP Function Code & Write-Lock Audit Harness
Standards: IEC 62443-4-2 & Modbus Application Protocol v1.1b3
"""

import argparse


class IcsModbusTamperHarness:
    READ_FCS = {0x01, 0x02, 0x03, 0x04}
    WRITE_FCS = {0x05, 0x06, 0x0F, 0x10, 0x17}

    def evaluate_frame(self, fc: int, addr: int, allow_writes: bool = False):
        if fc in self.READ_FCS:
            return True, "PASS"
        if fc in self.WRITE_FCS:
            if not allow_writes:
                return False, "REJECT_MODBUS_UNAUTHORIZED_WRITE"
            if 40000 <= (40001 + addr) <= 40100:
                return False, "REJECT_MODBUS_PROTECTED_REGISTER"
            return True, "PASS"
        return False, "REJECT_MODBUS_ILLEGAL_FC"

    def run_audit(self, mode="coil_override"):
        print("\033[1;36m================================================================================\033[0m")
        print("\033[1;37m   SENTINEL-LAB: MODBUS TCP FUNCTION CODE & WRITE-LOCK AUDIT                    \033[0m")
        print("\033[1;36m================================================================================\033[0m")
        steps = [
            ("Step 1 (Nominal Read Holding Regs)", 0x03, 0),
            ("Step 2 (Nominal Read Coils)", 0x01, 1),
        ]
        if mode == "illegal_fc":
            steps.append(("Step 3 (Diagnostic FC08 Restart Probe)", 0x08, 0))
        else:
            steps.append(("Step 3 (Unauthorized FC05 Coil Override)", 0x05, 1))

        for label, fc, addr in steps:
            ok, verdict = self.evaluate_frame(fc, addr)
            col = "\033[1;32m" if ok else "\033[1;31m"
            print(f"[*] {label:<42} | FC=0x{fc:02X} | {col}{verdict}\033[0m")
        print("\033[1;36m================================================================================\033[0m\n")


def main():
    parser = argparse.ArgumentParser(description="ICS Modbus TCP Tampering Audit")
    parser.add_argument("--mode", default="coil_override", choices=["coil_override", "illegal_fc"])
    args = parser.parse_args()
    IcsModbusTamperHarness().run_audit(mode=args.mode)


if __name__ == "__main__":
    main()
