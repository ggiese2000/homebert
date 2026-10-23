# Release-Readiness: 0.4.0

## Einordnung

**Version:** `0.4.0` (erste öffentliche HomeKit-Edition)
**Öffentlicher Quellstand:** frisches Repository mit einem unabhängigen
Initial-Commit für `0.4.0`; für die Quellcode-Veröffentlichung freigegeben.

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
von homeBert. Das ist keine absolute Sicherheitszusage. Der frische
Quellstand enthält nur synthetische Identitätstestwerte und eine unabhängige
Ein-Commit-Historie. Der finale Privacy-Scan fand keine bestätigten
Zugangsdaten, privaten Schlüssel, Firmware-Artefakte oder gerätespezifischen
Produktionswerte. Automatisches Secret-Scanning war in der Prüfungsumgebung
nicht installiert; die Prüfung ersetzt deshalb keine Vorsorge für künftige
Commits.

## Dokumentationsstatus

**READY WITH LIMITATIONS:** Die Grafik
`docs/assets/homeBert-Schnellübersicht.png` beschreibt die Bedienelemente und
den normalen LED-Ablauf zutreffend. Ihr Hinweis auf einen „konfigurierten
normalen LED-Modus“ meint ausschließlich Änderungen am Firmware-Quellcode mit
anschließendem Neubau; die Einrichtung bietet keine Modusauswahl. Das ist eine
redaktionelle Ambiguität, keine technische Freigabesperre. Für Details bleibt
`docs/LED_AND_BUTTON.md` maßgeblich.

Die Grafik wurde auf Veranlassung des Projektmaintainers mit OpenAI ChatGPT
für homeBert neu erstellt; eine vom Maintainer bereitgestellte Vorlage diente
als gestalterische Referenz. Der Maintainer bestätigt die Berechtigung zur
Verwendung und Veröffentlichung im öffentlichen homeBert-Repository. Die
Grafik ist keine offizielle Dokumentation des Hardwareherstellers.

## Lizenz und Veröffentlichungsstatus

**BLOCKED:** Die Binärweitergabe ist gemäß
[Lizenzierung und Weitergabe von Firmware](licensing-and-redistribution.md)
weiterhin **INCOMPLETE / NO-GO**. Offen sind insbesondere die genaue
Komponenten-/Archivzuordnung im Espressif-Paket, GCC/Newlib-Laufzeitnotices und
ein prüfbarer LGPL-2.1-Abschnitt-6-Ansatz. `THIRD_PARTY_NOTICES.md` ist daher
noch keine vollständige Beilage für Release-Binärdateien.

Der eigene Quellcode ist MIT-lizenziert. Nubert- und Apple-Bezeichnungen sind
Marken ihrer jeweiligen Inhaber; homeBert ist kein offizielles Produkt dieser
Unternehmen. Die Bluetooth-Protokollinformationen wurden durch Beobachtung und
Analyse der Kommunikation zwischen einem nuXinema Receiver und der zugehörigen
Steuerungs-App auf eigener Hardware ermittelt, nicht durch Dekompilierung von
Firmware oder App-Binärdateien. Eine externe Rechtsbegutachtung liegt nicht
vor; im geprüften Quellbaum wurde jedoch kein konkret identifiziertes fremdes
Quell- oder Protokollmaterial festgestellt.

## Nächste Schritte vor Veröffentlichung

- [ ] Vollständige Binärlizenz-Inventur und Release-Beilagen fertigstellen,
      falls Binärdateien später verteilt werden sollen.
- [ ] Zusätzliche Hardwarebeobachtungen (Power-/Volume-UI, WLAN-Fehler,
      BLE-Recovery) nach Bedarf nachholen und als Einschränkung dokumentieren;
      sie sind keine zusätzliche Release-Sperre.
- [ ] Finalen Kandidaten bauen, Prüfsummen erfassen und Privacy-/Link-Check
      wiederholen.
- [ ] Erst danach bewusst einen öffentlichen Tag, Assets oder einen öffentlichen
      Push freigeben.

## Gesamturteil

**Öffentliche Quellverteilung: FREIGEGEBEN.** Die LED-Formulierung in der
Grafik ist dabei nur redaktionell; Herkunft und
Veröffentlichungsberechtigung der Grafik sind dokumentiert.
**Binärverteilung: BLOCKED**, bis die separate Komponenten- und LGPL-Prüfung
abgeschlossen ist. **Funktionaler Firmwarestand: READY WITH LIMITATIONS.**
