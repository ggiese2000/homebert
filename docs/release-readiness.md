# Release-Readiness: 0.4.0

## Einordnung

**Vorgeschlagene Version:** `0.4.0` (erste öffentliche HomeKit-Edition)
**Firmware-Basis:** `b9c8f3a` und die in diesem Arbeitszweig folgenden
Dokumentations-/Release-Metadaten-Commits.

`0.4.0` ist ein angemessener Minor-Schritt innerhalb von SemVer vor 1.0: Es
fasst die erste öffentliche HomeKit-Edition zusammen. Ein Major-Sprung auf
`1.0.0` ist nicht begründet, weil Stabilität, Kompatibilität und Verfügbarkeit
nicht allgemein zugesichert werden. Unausgeführte Hardwaretests werden
offengelegt, sind aber keine zusätzliche Funktionssperre.

## Enthaltene, eingefrorene Funktionalität

- Lokale Einrichtung per BLE-/WLAN-Assistent, HomeKit-Setup-Code und QR-Code.
- Native HomeKit-Fernsehsteuerung für nuXinema preAV: ON/Standby, Eingänge und
  Lautstärke.
- BLE-bestätigte Zustandsübernahme; ausstehende Lautstärke- und
  Power-Anfragen werden gegenüber veralteten Rückmeldungen abgegrenzt.
- Automatische Surround-Zuordnung ausschließlich nach bestätigter,
  HomeKit-ausgelöster Eingangswahl.
- Zwei OTA-fähige Anwendungs-Partitionen, lokale Release-Images und
  Prüfsummen.
- LED-/Tasterzustände gemäß [LED und Taster](LED_AND_BUTTON.md).

## Test- und Hardwarestatus

| Bereich | Status |
|---|---|
| Build, Partitions- und Image-Prüfungen | **READY** – Release-Build 0.4.0 erfolgreich |
| Grundlegende WLAN-, BLE- und HomeKit-Nutzung | **READY WITH LIMITATIONS** – auf ATOM Lite/nuXinema preAV erprobt |
| Setup-Hotspot, Wizard und gelbes Setup-Blinken | **READY** – hardwarebeobachtet |
| Volume- und Power-Reconciliation | **READY WITH LIMITATIONS** – automatisiert geprüft; UI-Verhalten nach `b9c8f3a` ist noch nicht erneut auf Hardware beobachtet |
| WLAN-Fehlerbilder | **NOT YET VERIFIED** – Zustandstests vorhanden, keine abschließende Hardwarebeobachtung |
| BLE-Recovery nach vollständigem Receiver-Stromverlust | **NOT YET VERIFIED** – Diagnose- und Testpfade vorhanden, Produktionsstrategie nicht abgeschlossen |

## Varianten

| Variante | Status |
|---|---|
| Standard | **READY WITH LIMITATIONS** – die einzige im Repository buildbare Firmware |
| Advanced | **OUT OF SCOPE** – keine eigene Konfiguration oder kein eigenes Image in 0.4.0 |
| Home Assistant | **OUT OF SCOPE** – keine Integration in 0.4.0 |

## Sicherheit und Privatsphäre

Die Architektur arbeitet lokal über WLAN und BLE; es gibt keinen Cloud-Dienst
von homeBert. Das ist keine absolute Sicherheitszusage. Vor einer öffentlichen
Veröffentlichung ist ein erneuter Scan des finalen Git-Zustands erforderlich.

**BLOCKED:** In aktuellen Dateien und erreichbarer Historie befinden sich
gerätespezifische Identitäts-/Pairing-Testdaten. Werte werden hier absichtlich
nicht wiederholt. Vor einem öffentlichen Push müssen sie durch nicht
einsatzbezogene Testvektoren ersetzt und die bereits erreichbare Historie nach
einer gesonderten Freigabe bereinigt oder die betroffene Identität behandelt
werden. Allein das Entfernen aus dem Arbeitsbaum genügt nicht für eine
veröffentlichte Historie.

## Dokumentationsstatus

**READY:** Die korrigierte Grafik `docs/assets/homeBert-Schnellübersicht.png`
verweist auf die aktuellen LED- und Tasterfunktionen. Für Details bleibt
`docs/LED_AND_BUTTON.md` maßgeblich.

## Lizenz und Veröffentlichungsstatus

**BLOCKED:** Die Binärweitergabe ist gemäß
[Lizenzierung und Weitergabe von Firmware](licensing-and-redistribution.md)
weiterhin **INCOMPLETE / NO-GO**. Offen sind insbesondere die genaue
Komponenten-/Archivzuordnung im Espressif-Paket, GCC/Newlib-Laufzeitnotices und
ein prüfbarer LGPL-2.1-Abschnitt-6-Ansatz. `THIRD_PARTY_NOTICES.md` ist daher
noch keine vollständige Beilage für Release-Binärdateien.

Der eigene Quellcode ist MIT-lizenziert. Nubert- und Apple-Bezeichnungen sind
Marken ihrer jeweiligen Inhaber; homeBert ist kein offizielles Produkt dieser
Unternehmen. Die rechtliche Zulässigkeit einer Veröffentlichung
reverse-engineerter Protokollinformationen wurde nicht rechtlich begutachtet.

## Nächste Schritte vor Veröffentlichung

- [ ] Gerätebezogene Identitäts-/Pairing-Testdaten und gesamte erreichbare
      Historie mit ausdrücklicher Freigabe behandeln.
- [ ] Vollständige Binärlizenz-Inventur und Release-Beilagen fertigstellen.
- [ ] Zusätzliche Hardwarebeobachtungen (Power-/Volume-UI, WLAN-Fehler,
      BLE-Recovery) nach Bedarf nachholen und als Einschränkung dokumentieren;
      sie sind keine zusätzliche Release-Sperre.
- [ ] Finalen Kandidaten bauen, Prüfsummen erfassen und Privacy-/Link-Check
      wiederholen.
- [ ] Erst danach bewusst Version taggen, Assets veröffentlichen oder pushen.

## Gesamturteil

**Öffentliche Quell- und Binärverteilung: BLOCKED**, bis Privacy, Lizenzierung
und die fehlerhafte LED-Illustration abgearbeitet sind.
**Funktionaler Firmwarestand: READY WITH LIMITATIONS.**
