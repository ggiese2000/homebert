#!/usr/bin/env python3
"""Focused state vectors for asynchronous HomeKit volume confirmation."""

from pathlib import Path


class VolumeBridge:
    def __init__(self, value=52, valid=True):
        self.value = value
        self.valid = valid
        self.active = False
        self.write_sent = False
        self.target = 0
        self.last_confirmed = value
        self.published = []

    def request(self, value, accepted=True):
        if not accepted:
            return False
        self.active = True
        self.write_sent = False
        self.target = value
        self.last_confirmed = self.value
        return True

    def write_result(self, written):
        if not self.active:
            return
        if written:
            self.write_sent = True
        else:
            self.reconcile()

    def feedback(self, value):
        changed = not self.valid or self.value != value
        self.value = value
        self.valid = True
        if self.active:
            self.last_confirmed = value
            if self.write_sent and value == self.target:
                self.active = False
                self.published.append(value)
        elif changed:
            self.published.append(value)

    def reconcile(self):
        self.active = False
        self.write_sent = False
        if self.valid:
            self.published.append(self.value)

    def disconnect(self):
        self.active = False
        self.write_sent = False
        self.valid = False


def main():
    # A queued write is accepted by HomeKit, but transport success is not a
    # device confirmation.
    bridge = VolumeBridge()
    assert bridge.request(60)
    bridge.write_result(True)
    assert bridge.active and bridge.published == []

    # The known old value from a targeted status response is held while the
    # requested value is pending; it cannot move the slider backwards.
    bridge.feedback(52)
    assert bridge.active and bridge.published == []
    bridge.feedback(60)
    assert not bridge.active and bridge.published == [60]

    # Queue rejection remains a failed HomeKit write and changes no state.
    rejected = VolumeBridge()
    assert not rejected.request(60, accepted=False)
    assert not rejected.active and rejected.published == []

    # A failed GATT delivery immediately restores the last confirmed state.
    failed = VolumeBridge()
    assert failed.request(60)
    failed.write_result(False)
    assert not failed.active and failed.published == [52]

    # No feedback is bounded: reconciliation restores the last confirmed value.
    timeout = VolumeBridge()
    assert timeout.request(60)
    timeout.write_result(True)
    timeout.reconcile()
    assert timeout.published == [52]

    # Rapid slider movement coalesces at the command queue.  Feedback for the
    # superseded request remains internal until the newest target is confirmed.
    rapid = VolumeBridge()
    assert rapid.request(60)
    rapid.write_result(True)
    assert rapid.request(65)
    rapid.feedback(60)
    assert rapid.active and rapid.published == []
    rapid.write_result(True)
    rapid.feedback(65)
    assert rapid.published == [65]

    # A differing external change cannot be distinguished from delayed feedback
    # without protocol request IDs.  It is retained as authoritative and is
    # published at bounded reconciliation if the target never arrives.
    external = VolumeBridge()
    assert external.request(60)
    external.write_result(True)
    external.feedback(55)
    assert external.active and external.published == []
    external.reconcile()
    assert external.published == [55]

    # Disconnect never republishes cached data as a newly confirmed value.
    disconnected = VolumeBridge()
    assert disconnected.request(60)
    disconnected.write_result(True)
    disconnected.disconnect()
    disconnected.reconcile()
    assert disconnected.published == []

    source = (Path(__file__).parents[1] / "firmware" / "firmware.ino").read_text(encoding="utf-8")
    volume_service = source[source.index("struct HomeBertVolume"):source.index("struct HomeBertVolumeButtons")]
    assert "if (!queueNubertCommand(NubertCommand::kVolume, requested)) return false;" in volume_service
    assert "acceptHomeKitVolume(requested);" in volume_service
    assert "return true;" in volume_service
    assert "kVolumeConfirmationTimeoutMs" in source
    assert "homeKitVolumePending.writeSent" in source
    assert "void reconcilePendingHomeKitVolume()" in source
    assert "HomeKit volume confirmation timed out" in source
    assert "homeKitVolumePending = {};" in source


if __name__ == "__main__":
    main()
