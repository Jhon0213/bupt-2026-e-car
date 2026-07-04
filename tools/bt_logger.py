#!/usr/bin/env python3
"""Bluetooth serial CSV logger for open-loop speed tests."""

from __future__ import annotations

import argparse
import csv
import sys
from pathlib import Path

try:
    import serial
    from serial.tools import list_ports
except ImportError:  # pragma: no cover - depends on local environment
    serial = None
    list_ports = None


HEADER = [
    "t_ms",
    "mode",
    "test_id",
    "pwm_cmd",
    "left_speed",
    "right_speed",
    "left_count",
    "right_count",
]


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Log Bluetooth serial CSV data to a local CSV file."
    )
    parser.add_argument("--port", help="Bluetooth serial port, for example COM6")
    parser.add_argument("--baud", type=int, default=115200, help="Baud rate, default 115200")
    parser.add_argument(
        "--out",
        default="logs/open_loop_log.csv",
        help="Output CSV file, default logs/open_loop_log.csv",
    )
    parser.add_argument("--list", action="store_true", help="List available serial ports")
    return parser.parse_args()


def require_pyserial() -> None:
    if serial is None or list_ports is None:
        print("ERROR: pyserial is not installed. Install it with: pip install pyserial", file=sys.stderr)
        raise SystemExit(1)


def list_serial_ports() -> None:
    require_pyserial()
    ports = list(list_ports.comports())
    if not ports:
        print("No serial ports found.")
        return

    for port in ports:
        description = port.description or ""
        hwid = port.hwid or ""
        print(f"{port.device}\t{description}\t{hwid}")


def normalize_line(raw: bytes) -> str:
    return raw.decode("utf-8", errors="replace").strip()


def is_header_row(fields: list[str]) -> bool:
    return [field.strip() for field in fields] == HEADER


def parse_csv_line(line: str) -> list[str] | None:
    if not line:
        print("WARNING: empty line skipped")
        return None

    try:
        rows = list(csv.reader([line]))
    except csv.Error as exc:
        print(f"WARNING: CSV parse error skipped: {exc}; raw={line!r}")
        return None

    if not rows:
        print(f"WARNING: CSV parse produced no fields; raw={line!r}")
        return None

    fields = [field.strip() for field in rows[0]]
    if is_header_row(fields):
        return None

    if len(fields) != len(HEADER):
        print(f"WARNING: expected 8 fields, got {len(fields)}; raw={line!r}")
        return None

    return fields


def log_serial(port: str, baud: int, out_path: Path) -> None:
    require_pyserial()
    out_path.parent.mkdir(parents=True, exist_ok=True)

    print(f"Opening serial port {port} at {baud} baud...")
    print(f"Writing valid CSV rows to {out_path}")
    print("Press Ctrl+C to stop.")

    try:
        with serial.Serial(port=port, baudrate=baud, timeout=1) as ser, out_path.open(
            "w", newline="", encoding="utf-8"
        ) as csv_file:
            writer = csv.writer(csv_file)
            writer.writerow(HEADER)
            csv_file.flush()

            while True:
                try:
                    raw = ser.readline()
                except serial.SerialException as exc:
                    print(f"ERROR: serial read failed: {exc}", file=sys.stderr)
                    break

                if not raw:
                    continue

                line = normalize_line(raw)
                print(line)
                fields = parse_csv_line(line)
                if fields is None:
                    continue

                writer.writerow(fields)
                csv_file.flush()
    except serial.SerialException as exc:
        print(f"ERROR: cannot open or use serial port {port}: {exc}", file=sys.stderr)
        raise SystemExit(1)
    except KeyboardInterrupt:
        print("\nStopped by user. CSV file has been flushed safely.")


def main() -> None:
    args = parse_args()

    if args.list:
        list_serial_ports()
        return

    if not args.port:
        print("ERROR: --port is required unless --list is used.", file=sys.stderr)
        print("Example: python tools/bt_logger.py --port COM6 --baud 115200 --out logs/open_loop_log.csv")
        raise SystemExit(2)

    log_serial(args.port, args.baud, Path(args.out))


if __name__ == "__main__":
    main()
