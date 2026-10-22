#!/usr/bin/env python3
"""Deterministic state vectors for homeBert LED priority and GPIO39 gestures."""

from pathlib import Path


class Led:
    def __init__(self):
        self.startup_until = 500
        self.initial_blue_until = 0
        self.connected_pending = False
        self.green_until = 0
        self.command_until = 0

    @staticmethod
    def phase(now, half_period):
        return (now // half_period) % 2 == 0

    def start_ble(self, now, initial=True):
        if initial and not self.initial_blue_until:
            self.initial_blue_until = now + 2000

    def connected(self):
        self.connected_pending = True
        self.green_until = 0

    def color(self, now, *, setup=False, validation_failed=False, outage=False,
              wifi=True, ble=False, connecting=False, status=False):
        if validation_failed and setup:
            return "yellow" if self.phase(now, 500) else "red"
        if setup:
            return "yellow" if self.phase(now, 500) else "off"
        if outage:
            return "red" if self.phase(now, 500) else "off"
        if now < self.startup_until:
            return "white"
        if self.initial_blue_until and (now < self.initial_blue_until or connecting):
            return "blue" if self.phase(now, 250) else "off"
        if status:
            return "red" if not wifi else "green" if ble else "blue"
        if now < self.command_until:
            return "blue"
        if self.connected_pending and wifi and ble:
            self.connected_pending = False
            self.green_until = now + 5000
            return "green"
        if now < self.green_until:
            return "green"
        if wifi and connecting:
            return "blue" if self.phase(now, 250) else "off"
        return "off"


class Button:
    def __init__(self):
        self.pressed = False
        self.pending = False
        self.last_release = 0
        self.reset = self.status = self.reconnect = 0

    def press(self, now):
        self.pressed = True
        self.started = now

    def tick(self, now):
        if self.pressed and now - self.started >= 10000 and not self.reset:
            self.reset += 1
        if self.pending and now - self.last_release > 350:
            self.pending = False
            self.status += 1

    def release(self, now):
        duration = now - self.started
        self.pressed = False
        if self.reset or duration >= 10000:
            return
        if self.pending and now - self.last_release <= 350:
            self.pending = False
            self.reconnect += 1
        else:
            self.pending = True
            self.last_release = now


def main():
    # Normal boot: white, at least two seconds blue, then one green confirmation.
    led = Led()
    assert led.color(0, wifi=False) == "white"
    led.start_ble(500)
    led.connected()
    assert led.color(500, ble=True) == "blue"
    assert led.color(2499, ble=True) in ("blue", "off")
    assert led.color(2500, ble=True) == "green"
    assert led.color(7499, ble=True) == "green"
    assert led.color(7500, ble=True) == "off"
    assert led.color(9000, ble=True) == "off"  # polling cannot restart green

    # A slow connection stays blue until it actually connects; failure is never green.
    slow = Led()
    slow.start_ble(500)
    assert slow.color(3000, connecting=True, ble=False) in ("blue", "off")
    slow.connected()
    assert slow.color(3000, connecting=False, ble=True) == "green"
    failed = Led()
    failed.start_ble(500)
    assert failed.color(2600, connecting=False, ble=False) == "off"

    # Critical states preempt blue; commands cannot mask warnings.
    assert failed.color(1000, setup=True, connecting=True) == "yellow"
    assert failed.color(1000, setup=True, validation_failed=True, connecting=True) == "yellow"
    assert failed.color(1500, setup=True, validation_failed=True, connecting=True) == "red"
    assert failed.color(1000, outage=True, connecting=True) == "red"
    status = Led()
    status.command_until = 9200
    assert status.color(9100, wifi=True, ble=True) == "blue"
    assert status.color(9100, setup=True, wifi=True, ble=True) == "yellow"
    assert status.color(9300, wifi=True, ble=True) == "off"

    # Reconnect uses ordinary blue activity, then its own green confirmation.
    reconnect = Led()
    assert reconnect.color(1000, connecting=True, ble=False) in ("blue", "off")
    reconnect.connected()
    assert reconnect.color(1000, connecting=False, ble=True) == "green"
    assert reconnect.color(6000, ble=True) == "off"

    # Short, double and long presses retain their separate functions.
    button = Button()
    button.press(0); button.release(100); button.tick(451)
    assert button.status == 1 and button.reconnect == 0
    button = Button()
    button.press(0); button.release(50); button.press(200); button.release(250)
    assert button.reconnect == 1 and button.status == 0
    button = Button()
    button.press(0); button.tick(9999); button.release(9999); button.tick(10350)
    assert button.reset == 0 and button.status == 1
    button = Button()
    button.press(0); button.tick(10000); button.tick(20000); button.release(20001)
    assert button.reset == 1 and button.status == 0 and button.reconnect == 0

    source = (Path(__file__).parents[1] / "firmware" / "firmware.ino").read_text(encoding="utf-8")
    assert "constexpr uint32_t kInitialBleLedMinMs = 2000;" in source
    assert "constexpr uint32_t kConnectedLedConfirmationMs = 5000;" in source
    assert "initialBleAttempted" in source and "connectedLedPending" in source
    assert "kRebootHoldMinMs" not in source
    assert "kFactoryResetHoldMs = 10000" in source
    assert "kButtonDebounceMs" in source and "void requestBleReconnect()" in source
    assert "static_cast<int32_t>(now - initialBleLedUntil)" in source
    assert "static_cast<int32_t>(now - connectedLedUntil)" in source


if __name__ == "__main__":
    main()
