# Architektur (aktueller Entwicklungsstand)

The foundation uses Arduino-ESP32 3.3.8, HomeSpan 2.1.8, and NimBLE-Arduino
2.5.1. It uses no web framework, JSON library, filesystem, or application task.

- NVS stores `configured`, `wifi_verified`, `ssid`, and `password` in the
  `homebert` namespace.
- Without credentials, a temporary `homeBert Setup` AP serves the local setup
  wizard. Its active BLE scan processes callbacks without NimBLE's former
  eight-address cache limit and retains at most eight **filtered** selectable
  Nubert-service candidates. The AP is stopped before station mode begins.
- Setup saves the selected advertised BLE address and address type alongside
  Wi-Fi credentials. The address is an observed identity, not yet a verified
  stable Nubert identifier.
- First-time Wi-Fi validation has a 60-second limit. Failure returns to the
  setup AP; once `wifi_verified` is set, future outages only retry station
  connection and never reopen the setup AP.
- HomeSpan starts only after station mode obtains an IP address.
- NimBLE initializes once at boot and scans only while the setup page is open.
  Selection uses the observed Nubert advertising service UUID; it does not
  prove that every advertising product supports the nuXinema GATT protocol.
- After Wi-Fi and HomeSpan start, NimBLE creates one asynchronous client for
  the saved address/type. It logs services and characteristic UUID/properties
  once after connection and retries disconnections after five seconds.
- The verified nuXinema characteristic is read and subscribed after discovery.
  The main and sound-settings queries are queued once after each connection;
  serial `status` queues another main query. Standalone and bundled `86` TLV
  records update one central state model for power, volume, input, and surround.
  Values become invalid on disconnect and are not republished as verified.
- HomeSpan exposes one Television with linked input and speaker services, plus
  a same-accessory Fan `RotationSpeed` absolute-volume control. Fan `Active`
  is ignored and never affects receiver power. `DisplayOrder` keeps stable
  input identifiers while presenting Nubert's remote order. A HomeKit input
  change queues the mapped surround write only after that input is confirmed;
  remote and initial synchronization never trigger an automatic surround
  change. During an accepted HomeKit Power-ON transition only, an input request
  may be held until confirmed ON; it is bounded and cleared on timeout or
  disconnect. BLE notifications are published from the main loop, not the
  NimBLE callback. HomeKit writes are queued, then confirmed by notification
  or a delayed targeted status refresh.
- The central command guard permits input, volume, and surround only when ON.
  Power ON is permitted only from STANDBY and Power OFF / STANDBY only from ON.
  Both transitions await BLE confirmation.
- Heap metrics are emitted at boot, after NimBLE, after Wi-Fi, and after HomeSpan.

The OTA layout has two 1,920 KiB application slots. The remaining 192 KiB is
reserved but not mounted or written in Phase 1B.

Vor dem ersten HomeKit-Start leitet die Firmware Einrichtungscode und QR-
Setup-ID deterministisch aus der ESP32-Basis-MAC ab. Ein versionsgebundener
Migrationsmarker im `homebert`-Namensraum verhindert spätere SRP-
Neuberechnungen; HomeSpan bewahrt Verifikator, Accessory-Identität und
Controllerdaten separat. Ein Werksreset löscht den lokalen Namensraum und
ruft HomeSpans dokumentiertes Zurücksetzen der HAP-Pairing-Daten auf. Nicht
unterstützte Surround-Modi bleiben offene Arbeit.
