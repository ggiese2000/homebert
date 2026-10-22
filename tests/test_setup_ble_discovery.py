#!/usr/bin/env python3
"""Focused checks for bounded, UUID-filtered BLE setup discovery."""

from pathlib import Path


MAX_CANDIDATES = 8
MAX_NAME_LENGTH = 254
NUBERT_SERVICE = "0000a600-0000-1000-8000-00805f9b34fb"
MAX_WIFI_NETWORKS = 32


class Candidates:
    def __init__(self):
        self.items = []

    def result(self, services, name, address, address_type, rssi, connectable=True):
        # Model NimBLE's onResult callback: an active scan response has already
        # been merged before application filtering happens here.
        if not connectable or NUBERT_SERVICE not in {item.lower() for item in services}:
            return
        identity = (address, address_type)
        for item in self.items:
            if tuple(item[:2]) == identity:
                item[2:] = [name, rssi]
                return
        if len(self.items) < MAX_CANDIDATES:
            self.items.append([address, address_type, name, rssi])

    def select(self, index):
        return self.items[index] if 0 <= index < len(self.items) else None


class SetupSelection:
    """Model the RAM-only selection retained across a completed rescan."""

    def __init__(self):
        self.selected = None

    def choose(self, candidate):
        self.selected = candidate

    def rescan_completed(self, candidates):
        if self.selected is not None and not any(
            item[:2] == self.selected[:2] for item in candidates.items
        ):
            self.selected = None


class WiFiNetworks:
    """Model the fixed-capacity, strongest-SSID Wi-Fi setup list."""

    def __init__(self):
        self.items = []
        self.running = False
        self.failed = False
        self.truncated = False

    def start(self):
        if self.running:
            return False
        self.items = []
        self.truncated = False
        self.failed = False
        self.running = True
        return True

    def complete(self, records):
        assert self.running
        for ssid, rssi in records:
            if not ssid or len(ssid) > 32:
                continue
            for index, item in enumerate(self.items):
                if item[0] == ssid:
                    self.items[index] = (ssid, max(item[1], rssi))
                    break
            else:
                if len(self.items) < MAX_WIFI_NETWORKS:
                    self.items.append((ssid, rssi))
                else:
                    self.truncated = True
                    weakest = min(range(len(self.items)), key=lambda index: self.items[index][1])
                    if rssi > self.items[weakest][1]:
                        self.items[weakest] = (ssid, rssi)
        self.items.sort(key=lambda item: item[1], reverse=True)
        self.running = False

    def fail(self):
        assert self.running
        self.running = False
        self.failed = True


def resolved_ssid(manual, index, networks):
    """Manual text deliberately takes priority over an earlier radio selection."""
    if manual:
        return manual if len(manual) <= 32 else None
    if isinstance(index, int) and 0 <= index < len(networks):
        return networks[index][0]
    return None


class WiFiForm:
    """Model the small client-side selection state without a browser runtime."""

    def __init__(self):
        self.password = ""
        self.details = False
        self.connect_enabled = False
        self.visible = False

    def choose(self):
        self.details = True
        self.connect_enabled = True
        self.password = ""

    def toggle(self):
        self.visible = not self.visible

def main():
    # Raw advertisements never consume candidates before UUID filtering.
    scan = Candidates()
    for i in range(12):
        scan.result([], "", f"00:00:00:00:00:{i:02x}", 1, -90)
        scan.result(["180D"], "Other", f"00:00:00:00:01:{i:02x}", 1, -90)
        scan.result([NUBERT_SERVICE], "Other", f"00:00:00:00:02:{i:02x}", 1, -90,
                    connectable=False)
    scan.result([NUBERT_SERVICE], "Some Nubert model", "02:00:00:00:00:01", 1, -47)
    assert len(scan.items) == 1
    assert scan.select(0)[2] == "Some Nubert model"
    assert scan.select(1) is None

    # Advertising data and a name that arrive only in a later scan response remain eligible.
    response_name = Candidates()
    response_name.result([], "", "02:00:00:00:00:01", 1, -70)
    response_name.result([NUBERT_SERVICE], "Different device name", "02:00:00:00:00:01", 1, -46)
    assert len(response_name.items) == 1
    assert response_name.items[0][2:] == ["Different device name", -46]

    # A matching device without a name is deliberately selectable.
    unnamed = Candidates()
    unnamed.result([NUBERT_SERVICE.upper()], "", "02:00:00:00:00:01", 1, -46)
    assert unnamed.select(0)[2] == ""

    # Repeated advertisements update one candidate; address type is part of identity.
    dedupe = Candidates()
    dedupe.result([NUBERT_SERVICE], "First", "02:00:00:00:00:01", 1, -80)
    dedupe.result([NUBERT_SERVICE], "Renamed", "02:00:00:00:00:01", 1, -45)
    dedupe.result([NUBERT_SERVICE], "Different type", "02:00:00:00:00:01", 0, -45)
    assert len(dedupe.items) == 2
    assert dedupe.items[0][2:] == ["Renamed", -45]

    # The bounded list is capped only after valid Nubert candidates.
    bounded = Candidates()
    for i in range(MAX_CANDIDATES + 2):
        bounded.result([NUBERT_SERVICE], f"Device {i}", f"44:22:33:44:55:{i:02x}", 1, -50)
    assert len(bounded.items) == MAX_CANDIDATES
    assert bounded.select(MAX_CANDIDATES - 1) is not None
    assert bounded.select(MAX_CANDIDATES) is None

    # The explicit selection survives only when the same address and type return.
    selection = SetupSelection()
    selection.choose(scan.select(0))
    scan.result([NUBERT_SERVICE], "Updated name", "02:00:00:00:00:01", 1, -44)
    selection.rescan_completed(scan)
    assert selection.selected is not None
    missing_after_rescan = Candidates()
    missing_after_rescan.result([NUBERT_SERVICE], "Other", "02:00:00:00:00:02", 1, -44)
    selection.rescan_completed(missing_after_rescan)
    assert selection.selected is None

    wifi = WiFiNetworks()
    assert wifi.start()
    assert not wifi.start()  # no concurrent scan request
    wifi.complete([("Weak", -82), ("Strong", -42), ("Strong", -58), ("", -20)])
    assert wifi.items == [("Strong", -42), ("Weak", -82)]
    assert resolved_ssid("", 0, wifi.items) == "Strong"
    assert resolved_ssid("Hidden network", 0, wifi.items) == "Hidden network"
    assert resolved_ssid("", "other", wifi.items) is None

    # Selecting an SSID clears an earlier credential; visibility never changes
    # the credential, while changing the selected network clears it again.
    form = WiFiForm()
    form.choose()
    form.password = "first-password"
    form.toggle()
    assert form.password == "first-password" and form.visible
    form.choose()
    assert form.details and form.connect_enabled and form.password == ""
    form.password = "second-password"
    form.choose()
    assert form.details and form.connect_enabled and form.password == ""

    assert wifi.start()  # a completed scan can be repeated and replaces stale results
    wifi.complete([(f"SSID {i}", -30 - i) for i in range(MAX_WIFI_NETWORKS)] +
                  [("Strong late network", -20), ("Weak late network", -95)])
    assert len(wifi.items) == MAX_WIFI_NETWORKS
    assert wifi.truncated
    assert wifi.items[0] == ("Strong late network", -20)

    failed_wifi = WiFiNetworks()
    assert failed_wifi.start()
    failed_wifi.fail()
    assert failed_wifi.failed and not failed_wifi.running

    source = (Path(__file__).parents[1] / "firmware" / "firmware.ino").read_text(encoding="utf-8")
    setup_region = source[source.index("void startBleScan("):source.index("void printDiagnosticScanResults()")]
    assert "bleScan->setScanCallbacks(&setupScanCallbacks);" in setup_region
    assert "bleScan->setMaxResults(0);" in setup_region
    assert "class SetupScanCallbacks" in source
    callback_region = source[source.index("class SetupScanCallbacks"):source.index("void showHomeBertDiagnostics")]
    assert "void onResult(" in callback_region
    assert "device->isConnectable()" in callback_region
    assert "device->isAdvertisingService(nubertAdvertisingService)" in callback_region
    assert "findSetupBleCandidate" in callback_region
    assert "setupBleCandidateCount >= kMaxBleScanResults" in callback_region
    assert "kNubertAdvertisingServiceUuid" in source
    assert "kNubertServiceUuid" not in callback_region
    setup_page = source[source.index("bool parseBleIndex("):source.index("void handleBleRescan()")]
    assert "parsed >= setupBleCandidateCount" in setup_page
    assert "NimBLEScanResults results" not in setup_page
    assert "Welcome to homeBert" in setup_page
    assert "Start setup" in setup_page
    welcome_region = source[source.index("void handleSetupRoot()"):source.index("void handleSetupBle()")]
    assert "task-icon>1" in welcome_region and "task-icon>2" in welcome_region
    assert "✓" not in welcome_region
    assert "void handleSetupBle()" in setup_page
    assert "Select your Nubert device" in setup_page
    assert "Searching for Nubert devices" in setup_page
    assert "Scanning for Nubert devices..." in setup_page
    assert "Nubert app completely before scanning" in setup_page
    assert "Nubert device (name unavailable)" in setup_page
    assert "No Nubert devices found. Make sure your device is powered on and the Nubert app is completely closed, then" in setup_page
    assert "More than eight matching Nubert devices were found; showing the first eight discovered." in setup_page
    assert "setupBleCandidatesTruncated = true" in callback_region
    assert "void appendSetupPageStart" in setup_page
    assert "class=progress" in setup_page
    assert "Start</span>" in setup_page
    assert "Bluetooth</span>" in setup_page
    assert "Wi-Fi</span>" in setup_page
    assert "Finish</span>" in setup_page
    progress_region = source[source.index("void appendSetupPageEnd"):source.index("void appendSetupBleNotice")]
    assert "stage == 0" in progress_region
    assert "stage == 1" in progress_region
    assert "stage == 2" in progress_region
    assert "stage == 3" in progress_region
    assert "aria-current=step" in progress_region
    assert "class=progress-row" in progress_region
    assert "class=progress-track" in progress_region
    assert "stage >= 1" in progress_region
    assert "stage >= 2" in progress_region
    assert "stage >= 3" in progress_region
    assert ".milestone:before" not in source
    assert ".progress-track{display:flex" in source
    assert "<form method=post action=/select>" in setup_page
    assert "void handleSetupSelection()" in setup_page
    assert "Location\", \"/wifi" in setup_page
    assert "void handleSetupWiFi()" in setup_page
    assert "<form method=post action=/save>" in setup_page
    assert "Rescan" in setup_page
    assert "Scanning for Nubert devices..." in setup_page
    assert "void validateSetupBleSelection()" in source
    assert "setupBleSelectionMatches" in source
    assert "clearSetupBleSelection();" in source
    assert 'setupServer.on("/select", HTTP_POST, handleSetupSelection);' in source
    assert 'setupServer.on("/ble", HTTP_GET, handleSetupBle);' in source
    assert 'setupServer.on("/wifi", HTTP_GET, handleSetupWiFi);' in source
    assert 'setupServer.on("/wifi-rescan", HTTP_GET, handleSetupWiFiRescan);' in source
    assert 'setupServer.on("/homebert-icon.webp", HTTP_GET, handleSetupIcon);' in source
    assert "hasNuXinemaName" not in source
    assert "restoreNormalBleScanConfiguration();" in setup_region
    diagnostic_region = source[source.index("void printDiagnosticScanResults()"):source.index("void finishDiagnosticBleScan(")]
    assert "getResults()" in diagnostic_region

    wifi_region = source[source.index("void clearSetupWiFiNetworks()"):source.index("int findSetupBleCandidate")]
    assert "WiFi.scanNetworks(true, false, false, 120)" in wifi_region
    assert "WiFi.scanComplete()" in wifi_region
    assert "WiFi.scanDelete()" in wifi_region
    assert "kMaxSetupWiFiNetworks" in wifi_region
    assert "sortSetupWiFiNetworks" in wifi_region
    assert "setupPortalRunning" in wifi_region
    assert "NimBLE" not in wifi_region
    assert "homeSpan" not in wifi_region
    assert "Other / Hidden Network" in setup_page
    assert "Rescan networks" in setup_page
    assert "Show password" in setup_page
    assert "selectedSetupWiFiSsid" in setup_page
    assert "Wi-Fi scan could not complete" in setup_page
    assert "Setup complete" in setup_page
    assert "homeBert is ready to add to Apple Home." in setup_page
    assert "This page can now be closed." in setup_page
    assert "image/webp" in source
    assert "kSetupConfirmationDisplayMs" in source
    assert "text/html; charset=utf-8" in source
    assert "Â" not in source
    assert "Nubert · HomeKit Bridge" not in source
    assert "RSSI ·" not in source
    assert ".networks{max-height:188px;overflow:auto" in source
    assert "-webkit-overflow-scrolling:touch" in source
    assert "Scroll for more" in setup_page
    assert "setupWiFiNetworkCount > 4" in setup_page
    assert "<div id=manual hidden>" in setup_page
    assert "id=ssid name=ssid" in setup_page
    assert "id=password name=password" in setup_page
    assert source.count("id=password") == 1
    assert "Change network" not in setup_page
    assert "Selected network<strong>" in setup_page
    assert "Searching for Wi-Fi networks" in setup_page
    assert "Scanning for Wi-Fi networks..." in setup_page
    assert "Looking for nearby 2.4 GHz networks." in setup_page
    assert "Continue to Wi-Fi</button>" in setup_page
    assert "Scan again" in setup_page
    assert "btn tertiary" in setup_page
    assert "class=nav" in setup_page
    assert "justify-content:space-between" in source
    assert "@keyframes scan" in source
    assert "p.value=''" in setup_page
    assert "d('ssid').disabled=1" in setup_page
    assert "d('ssid').disabled=0" in setup_page
    assert "function select()" in setup_page
    assert "d('network-list').hidden=1" not in setup_page
    assert "id=connect class=" in setup_page and "btn primary" in setup_page
    assert "class=password-tools" in setup_page
    assert ".password-tools{display:flex;justify-content:flex-end" in source
    assert "function change()" not in setup_page
    password_index = setup_page.index("id=password")
    password_tools_index = setup_page.index("class=password-tools")
    other_network_index = setup_page.index("class=form-options")
    navigation_index = setup_page.index("nav form-nav")
    assert password_index < password_tools_index < other_network_index < navigation_index
    assert '<a class=btn tertiary href=/ble>Back</a><button id=connect class=\\"btn primary\\"' in setup_page
    assert ".form-nav{margin-top:24px;padding-top:18px;border-top:1px solid #dfe5ea}" in source
    final_region = source[source.index("void handleSetupSave()"):source.index("void handleBleRescan()")]
    assert "appendSetupPageStart(page, 3)" in final_region
    assert "appendSetupPageEnd(page, 3)" in final_region
    assert "homeKitQrPayload(qrPayload)" in final_region
    assert "appendHomeKitQrSvg(page, qrPayload)" in final_region
    assert "class=homekit-complete" in final_region
    assert "class=success-icon" in final_region
    assert "Setup complete" in final_region
    assert "class=homekit-pairing" in final_region
    assert "HomeKit setup code" in final_region
    assert "Copy code" in final_region
    assert "This page can now be closed." in final_region
    assert "class=close-message" in final_region
    assert final_region.index("HomeKit setup code") < final_region.index("id=homekit-code")
    assert final_region.index("id=homekit-code") < final_region.index("Copy code")
    assert final_region.index("Copy code") < final_region.index("appendHomeKitQrSvg(page, qrPayload)")
    assert final_region.index("appendHomeKitQrSvg(page, qrPayload)") < final_region.index("Open Apple Home and enter this code")
    assert "appendHomeKitQrSvg" in final_region  # final QR is fully embedded in the response
    assert "<qrcode.h>" in source
    assert "ESP_QRCODE_ECC_MED" in source
    assert "shape-rendering=" in source and "crispEdges" in source
    assert "button.btn.primary,a.btn.primary" in source
    assert "background:#178491;color:#fff" in source
    assert "button.btn.tertiary,a.btn.tertiary" in source
    assert "-webkit-appearance:none" in source
    assert ".homekit-complete,.homekit-pairing{text-align:center}" in source
    assert "font-variant-numeric:tabular-nums" in source
    assert "Connect to Wi-Fi</button>" not in setup_page
    assert "Next</button>" in setup_page
    bluetooth_scan = source[source.index("Searching for Nubert devices"):source.index("void handleSetupSelection()")]
    assert 'class=\\"scan-icon bluetooth-icon\\" viewBox=\\"-1 -1 26 26\\"' in bluetooth_scan
    assert "M12 2v20l6-6-6-6 6-6-6-6zm0 10L6 6m0 12 6-6" in bluetooth_scan
    assert "stroke-linejoin=" in bluetooth_scan and "round" in bluetooth_scan
    assert ".bluetooth-icon{width:32px;height:32px;overflow:visible}" in source
    asset = Path(__file__).parents[1] / "docs" / "assets" / "homebert-icon-192.webp"
    assert asset.read_bytes()[:12] == b"RIFF0\x0b\x00\x00WEBP"
    assert asset.stat().st_size == 2872
    icon_header = (Path(__file__).parents[1] / "firmware" / "homebert_icon.h").read_text(encoding="utf-8")
    assert "kHomeBertIconWebp" in icon_header
    assert "kHomeBertIconWebpLength" in icon_header


if __name__ == "__main__":
    main()
