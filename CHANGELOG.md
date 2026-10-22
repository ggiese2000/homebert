# Changelog

Alle relevanten Änderungen folgen [Semantic Versioning](https://semver.org/).

## [0.4.0] - Unreleased

- Native HomeKit-Integration für nuXinema preAV mit Power ON/Standby,
  Eingangswahl, Fan-basierter Lautstärke und BLE-bestätigter Synchronisierung.
- Der lokale Einrichtungsassistent führt durch Nubert-BLE-Auswahl,
  WLAN-Konfiguration und HomeKit-Pairing mit QR-Code.
- Eingangswahl kann den bestätigten Power-ON-Übergang sicher abwarten;
  Surround wird danach gezielt auf Auto oder Stereo gesetzt.
- Ausstehende Lautstärke- und Power-Anfragen werden gegenüber verzögerten
  BLE-Statusmeldungen abgeglichen, damit Apple Home nicht kurz zurückspringt.
- LED-/Tasterverhalten wurde für Start, Verbindungsstatus, BLE-Reconnect und
  Werksreset vereinheitlicht.
- Lokaler Release-Ablauf erzeugt vollständige und App-only-Images,
  Prüfsummen sowie Build-Provenienz.

## [0.3.6] - Unreleased

- Die serielle Konsole verwendet nun ausschließlich die native HomeSpan-CLI;
  homeBert registriert die kompakten Zusatzbefehle `@n` und `@p`.
- HomeKit-Einrichtungscode und QR-Setup-ID werden nun deterministisch aus der
  ESP32-Basis-MAC abgeleitet; die einmalige SRP-Migration bewahrt bestehende
  Controller-, WLAN- und BLE-Daten.

## [0.3.5] - Unreleased

- HomeKit-Eingangswahl kann während einer bestätigten Power-ON-Transition
  sicher bis zur ON-Rückmeldung zurückgestellt werden.
- Die HomeKit-Eingangsliste folgt der dokumentierten Reihenfolge der
  nuXinema-preAV-Fernbedienung, ohne BLE-Zuordnungen zu ändern.
- Die absolute Lautstärke wird als HomeKit-Fan-Geschwindigkeit statt als Licht
  dargestellt, damit sie nicht an Lichtgruppenaktionen teilnimmt.
