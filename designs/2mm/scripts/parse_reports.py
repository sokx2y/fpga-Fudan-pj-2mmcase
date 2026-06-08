#!/usr/bin/env python3
"""Summarize HLS and Vivado reports for the 2mm project."""

from __future__ import annotations

import argparse
import json
import re
from pathlib import Path
from typing import Any


FIXED_PART = "xc7k325tffv900-2"


def read_text(path: Path | None) -> str:
    if path is None or not path.exists():
        return ""

    return path.read_text(encoding="utf-8", errors="replace")


def find_first(paths: list[Path], patterns: tuple[str, ...]) -> Path | None:
    for pattern in patterns:
        matches = sorted(path for root in paths for path in root.rglob(pattern))
        if matches:
            return matches[0]

    return None


def parse_number(pattern: str, text: str) -> float | None:
    match = re.search(pattern, text, re.IGNORECASE | re.MULTILINE)
    if match is None:
        return None

    return float(match.group(1))


def parse_int(pattern: str, text: str) -> int | None:
    value = parse_number(pattern, text)
    if value is None:
        return None

    return int(value)


def parse_hls_latency_row(text: str) -> tuple[int | None, int | None]:
    patterns = (
        r"\|\s*Latency\s*\(cycles\)\s*\|[^\n]*\n\|[^\n]*\n\|\s*(\d+)\s*\|\s*(\d+)",
        r"\bLatency\s*\(cycles\).*?\|\s*(\d+)\s*\|\s*(\d+)",
    )

    for pattern in patterns:
        match = re.search(pattern, text, re.IGNORECASE | re.DOTALL)
        if match:
            return int(match.group(1)), int(match.group(2))

    return None, None


def parse_hls_interval_row(text: str) -> tuple[int | None, int | None]:
    patterns = (
        r"\|\s*Interval\s*\(cycles\)\s*\|[^\n]*\n\|[^\n]*\n\|[^\n]*\|\s*(\d+)\s*\|\s*(\d+)",
        r"\bInterval\s*\(cycles\).*?\|\s*(\d+)\s*\|\s*(\d+)",
    )

    for pattern in patterns:
        match = re.search(pattern, text, re.IGNORECASE | re.DOTALL)
        if match:
            return int(match.group(1)), int(match.group(2))

    return None, None


def parse_hls_report(text: str) -> dict[str, Any]:
    latency_min, latency_max = parse_hls_latency_row(text)
    interval_min, interval_max = parse_hls_interval_row(text)

    return {
        "latency_cycles_min": latency_min,
        "latency_cycles_max": latency_max,
        "interval_cycles_min": interval_min,
        "interval_cycles_max": interval_max,
        "estimated_clock_period_ns": parse_number(r"Estimated Clock Period\s*:\s*([0-9.]+)", text),
        "resources": {
            "BRAM": parse_int(r"\|\s*BRAM_18K\s*\|\s*(\d+)", text),
            "DSP": parse_int(r"\|\s*DSP48E\s*\|\s*(\d+)", text),
            "FF": parse_int(r"\|\s*FF\s*\|\s*(\d+)", text),
            "LUT": parse_int(r"\|\s*LUT\s*\|\s*(\d+)", text),
        },
    }


def parse_timing_report(text: str) -> dict[str, Any]:
    return {
        "wns_ns": parse_number(r"\bWNS(?:\([^)]*\))?\s*\|?\s*([-+]?[0-9.]+)", text),
        "tns_ns": parse_number(r"\bTNS(?:\([^)]*\))?\s*\|?\s*([-+]?[0-9.]+)", text),
        "post_route_clock_period_ns": parse_number(r"Requirement:\s*([0-9.]+)ns", text),
    }


def parse_utilization_report(text: str) -> dict[str, Any]:
    return {
        "BRAM": parse_int(r"\|\s*Block RAM Tile\s*\|\s*(\d+)", text),
        "DSP": parse_int(r"\|\s*DSPs\s*\|\s*(\d+)", text),
        "FF": parse_int(r"\|\s*Slice Registers\s*\|\s*(\d+)", text),
        "LUT": parse_int(r"\|\s*Slice LUTs\s*\|\s*(\d+)", text),
    }


def summarize(report_dir: Path, hls: Path | None, timing: Path | None, util: Path | None) -> dict[str, Any]:
    roots = [report_dir]
    hls_report = hls or find_first(roots, ("*csynth*.rpt",))
    timing_report = timing or find_first(roots, ("*timing*.rpt", "*timing_summary*.rpt"))
    utilization_report = util or find_first(roots, ("*utilization*.rpt", "*util*.rpt"))

    return {
        "part": FIXED_PART,
        "report_dir": str(report_dir),
        "reports": {
            "hls_csynth": str(hls_report) if hls_report else None,
            "vivado_timing": str(timing_report) if timing_report else None,
            "vivado_utilization": str(utilization_report) if utilization_report else None,
        },
        "hls": parse_hls_report(read_text(hls_report)),
        "vivado_timing": parse_timing_report(read_text(timing_report)),
        "vivado_utilization": parse_utilization_report(read_text(utilization_report)),
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("report_dir", type=Path)
    parser.add_argument("--hls-report", type=Path)
    parser.add_argument("--timing-report", type=Path)
    parser.add_argument("--utilization-report", type=Path)
    args = parser.parse_args()

    summary = summarize(
        report_dir=args.report_dir,
        hls=args.hls_report,
        timing=args.timing_report,
        util=args.utilization_report,
    )
    print(json.dumps(summary, indent=2, sort_keys=True))

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
