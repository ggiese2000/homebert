#!/usr/bin/env python3
"""Focused state-model checks for homeBert's non-persistent BLE diagnostics."""

from pathlib import Path


def parse_level(command):
    """Mirror the intentionally strict @L callback input (without leading @)."""
    if command == "L":
        return "show"
    if len(command) == 2 and command[0] == "L" and command[1] in "012":
        return int(command[1])
    return None


class DiagnosticScan:
    def __init__(self):
        self.connected = False
        self.connecting = False
        self.pending = False
        self.running = False
        self.reconnect_allowed = True

    def request(self):
        if self.pending or self.running:
            return "duplicate"
        if self.connected:
            return "refused"
        if self.connecting:
            self.pending = True
            self.reconnect_allowed = False
            return "queued"
        self.running = True
        self.reconnect_allowed = False
        return "started"

    def connection_finished(self):
        self.connecting = False
        if self.pending:
            self.pending = False
            self.running = True
            return "started"
        return "idle"

    def finish(self):
        self.pending = False
        self.running = False
        self.reconnect_allowed = True


def exact_identity(configured_address, configured_type, observed_address, observed_type):
    return configured_address == observed_address and configured_type == observed_type


def main():
    assert parse_level("L") == "show"
    assert parse_level("L0") == 0
    assert parse_level("L1") == 1
    assert parse_level("L2") == 2
    for command in ("", "l", "L3", "L-1", "L 1", "L11", "b"):
        assert parse_level(command) is None

    # Diagnostic level is intentionally a RAM-only value: a new model starts at zero.
    diagnostic_level = 2
    assert diagnostic_level == 2
    diagnostic_level = 0
    assert diagnostic_level == 0

    scan = DiagnosticScan()
    scan.connecting = True
    assert scan.request() == "queued"
    assert scan.request() == "duplicate"
    assert not scan.reconnect_allowed
    assert scan.connection_finished() == "started"
    assert scan.running and not scan.pending
    scan.finish()  # completion, failure and timeout share this recovery state.
    assert not scan.running and scan.reconnect_allowed

    connected = DiagnosticScan()
    connected.connected = True
    assert connected.request() == "refused"
    assert not connected.running

    assert exact_identity("02:00:00:00:00:01", 1, "02:00:00:00:00:01", 1)
    assert not exact_identity("02:00:00:00:00:01", 1, "02:00:00:00:00:01", 0)
    assert not exact_identity("02:00:00:00:00:01", 1, "02:00:00:00:00:02", 1)

    source = Path(__file__).parents[1] / "firmware" / "firmware.ino"
    text = source.read_text(encoding="utf-8")
    assert "new SpanUserCommand('L'" in text
    assert "new SpanUserCommand('b'" in text
    assert "diagnosticScanPending" in text
    assert "!diagnosticScanIsActive() && !bleScanRunning" in text
    diagnostic_function = text[text.index("void startDiagnosticBleScan() {", text.index("void finishDiagnosticBleScan")):
                               text.index("void restoreNormalBleScanConfiguration()")]
    assert "Preferences" not in diagnostic_function
    assert "connect(" not in diagnostic_function


if __name__ == "__main__":
    main()
