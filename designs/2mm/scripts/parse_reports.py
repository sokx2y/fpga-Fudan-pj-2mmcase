#!/usr/bin/env python3
"""Parse HLS/Vivado reports for the 2mm project.

This is a lightweight template. Extend it as report locations are stabilized.
The project target part is fixed to xc7k325tffv900-2.
"""

from __future__ import annotations

import argparse
import json
import re
from pathlib import Path


FIXED_PART = "xc7k325tffv900-2"


def read_text(path: Path) -> str:
    return path.read_text(encoding="utf-8", errors="replace")


def parse_first_number(pattern: str, text: str) -> float | None:
    match = re.search(pattern, text, re.IGNORECASE | re.MULTILINE)
    if not match:
        return None
    return float(match.group(1))


def parse_reports(report_dir: Path) -> dict[str, object]:
    timing_files = list(report_dir.rglob("*timing*.rpt"))
    util_files = list(report_dir.rglob("*util*.rpt"))
    hls_files = list(report_dir.rglob("*csynth*.rpt"))

    result: dict[str, object] = {
        "part": FIXED_PART,
        "report_dir": str(report_dir),
        "latency_cycles": None,
        "post_route_clock_period_ns": None,
        "wns": None,
        "resources": {
            "DSP": None,
            "BRAM": None,
            "LUT": None,
            "FF": None
        }
    }

    if timing_files:
        timing_text = read_text(timing_files[0])
        result["wns"] = parse_first_number(r"\bWNS\b[^\n\r-+]*([-+]?\d+(?:\.\d+)?)", timing_text)

    if hls_files:
        hls_text = read_text(hls_files[0])
        result["latency_cycles"] = parse_first_number(r"Latency[^\n\r]*?(\d+)", hls_text)

    if util_files:
        util_text = read_text(util_files[0])
        for key in ("DSP", "BRAM", "LUT", "FF"):
            result["resources"][key] = parse_first_number(rf"\b{key}\b[^\n\r]*?(\d+)", util_text)

    return result


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("report_dir", type=Path)
    parser.add_argument("--part", default=FIXED_PART)
    args = parser.parse_args()

    if args.part != FIXED_PART:
        raise SystemExit(f"ERROR: project part is fixed to {FIXED_PART}; got {args.part}")

    print(json.dumps(parse_reports(args.report_dir), indent=2, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

