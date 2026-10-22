#!/usr/bin/env python3
"""Focused state vectors for asynchronous HomeKit power confirmation."""

from pathlib import Path


class PowerBridge:
    def __init__(self, state="standby"):
        self.state = state
        self.active = False
        self.write_sent = False
        self.target = None
        self.last_confirmed = state
        self.published = []
        self.queued = []

    def request(self, target, allowed=True):
        if not allowed or self.active:
            return False
        self.active = True
        self.write_sent = False
        self.target = target
        self.last_confirmed = self.state
        self.queued.append(target)
        return True

    def write_result(self, written):
        if not self.active:
            return
        if written:
            self.write_sent = True
        else:
            self.reconcile()

    def feedback(self, state):
        self.state = state
        self.last_confirmed = state
        if self.active and self.write_sent and state == self.target:
            self.active = False
            self.published.append(state)

    def reconcile(self):
        if not self.active:
            return
        self.active = False
        self.state = self.last_confirmed
        self.published.append(self.state)

    def disconnect(self):
        self.active = False


def main():
    # Accepted ON/OFF writes retain the requested HomeKit value pending actual
    # receiver feedback; transport acknowledgement alone is not confirmation.
    on = PowerBridge("standby")
    assert on.request("on")
    on.write_result(True)
    assert on.active and on.published == []
    on.feedback("standby")  # stale status cannot roll the UI back
    assert on.active and on.published == []
    on.feedback("on")
    assert not on.active and on.published == ["on"]

    off = PowerBridge("on")
    assert off.request("standby")
    off.write_result(True)
    off.feedback("on")
    assert off.active and off.published == []
    off.feedback("standby")
    assert off.published == ["standby"]

    # Queue rejection, transport failure, timeout and disconnect never invent
    # a successful transition or a second command.
    rejected = PowerBridge("standby")
    assert not rejected.request("on", allowed=False)
    assert rejected.queued == [] and rejected.published == []
    failed = PowerBridge("on")
    assert failed.request("standby")
    failed.write_result(False)
    assert failed.published == ["on"]
    timed_out = PowerBridge("standby")
    assert timed_out.request("on")
    timed_out.write_result(True)
    timed_out.reconcile()
    assert timed_out.published == ["standby"]
    disconnected = PowerBridge("on")
    assert disconnected.request("standby")
    disconnected.write_result(True)
    disconnected.disconnect()
    assert not disconnected.active and disconnected.queued == ["standby"]

    # Rapid contradictory requests are rejected while the first transition is
    # pending, preventing duplicate writes and unintended wake-ups.
    rapid = PowerBridge("standby")
    assert rapid.request("on")
    assert not rapid.request("standby")
    assert rapid.queued == ["on"]

    source = (Path(__file__).parents[1] / "firmware" / "firmware.ino").read_text(encoding="utf-8")
    television = source[source.index("struct HomeBertTelevision"):source.index("struct HomeBertVolume")]
    assert "struct HomeKitPowerPending" in source
    assert "acceptHomeKitPower(NubertPowerState::kOn);" in television
    assert "acceptHomeKitPower(NubertPowerState::kStandby);" in television
    assert "return true;" in television
    assert "return false;" in television
    assert "void reconcilePendingHomeKitPower()" in source
    assert "HomeKit power confirmation timed out" in source
    assert "homeKitPowerPending.writeSent" in source
    assert "HomeKitVolumePending" in source  # volume reconciliation remains present


if __name__ == "__main__":
    main()
