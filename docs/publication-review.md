# Privacy- und Lizenzprüfung für die Veröffentlichung

**Stand:** homeBert 0.4.0, bereinigter öffentlicher Staging-Baum. Dieses Dokument ist
ein technischer Prüfbericht, kein Rechtsgutachten und keine
Compliance-Erklärung.

## Ergebnis

| Vorhaben | Status | Begründung |
|---|---|---|
| Quellcode veröffentlichen | **FREIGEGEBEN** | Der Quellbaum ist privacy-bereinigt; Herkunft und Veröffentlichungsberechtigung der bereitgestellten Hardware-Grafik sind dokumentiert. Die Binärweitergabe ist davon getrennt weiterhin nicht freigegeben. |
| Firmware-Binärdateien veröffentlichen | **BLOCKED** | Komponenten- und Lizenzpflichten für das konkrete Image sind nicht vollständig geklärt. |
| Private Entwicklung | **READY WITH LIMITATIONS** | Keine Veröffentlichung erfolgt; offene Punkte bleiben nachvollziehbar dokumentiert. |

## Privacy-Audit

### Umfang

Geprüft wurden der getrackte Staging-Baum, seine frische lokale Historie,
Build-/Release-Skripte, Tests, Dokumentation, Git-LFS-Konfiguration
sowie potenzielle Binär- und Logdateinamen. Es gibt keine getrackte CI- oder
Git-LFS-Konfiguration und keine getrackten Firmware-, ELF-, Map-, PCAP- oder
Log-Artefakte. Die ignorierten lokalen Toolchain- und Build-Verzeichnisse
werden nicht veröffentlicht.

### Befunde (Werte absichtlich redigiert)

| Schwere | Ort | Kategorie | Erforderliche Behandlung |
|---|---|---|---|
| Erledigt | `docs/homekit-identity.md` | Konkrete Hardware-/HomeKit-Identitätsbeispiele | Durch generische Beschreibung und synthetische Testvektoren ersetzt. |
| Erledigt | HomeKit-/BLE-Tests | Gerätebezogene MAC-, Setup- oder BLE-Testwerte | Durch klar synthetische Fixtures ersetzt; Bedeutung und Abdeckung erhalten. |
| Erledigt | Temporärer `@r`-Recovery-Test | Fest verdrahtete Gerätekennung in Entwicklungsinstrumentierung | Aus dem Staging-Baum samt Dokumentation und Test entfernt; normaler Reconnect und `@b`-Diagnose bleiben erhalten. |
| Privat archiviert | Ursprüngliche Entwicklungshistorie | Historische Kopien und Diagnosekontext | Nicht Teil der frischen öffentlichen Staging-Historie. |

Die Prüfung hat in getrackten Dateien und der einzigen erreichbaren Historie
keine bestätigten WLAN-Passwörter, privaten Schlüsselblöcke, API-Tokens oder
eingecheckten Build-Images gefunden. Das ist keine Garantie für künftige
Commits oder nicht erreichbare Git-Objekte.

### Archiv- und Veröffentlichungsgrenze

Der ursprüngliche Baum bleibt als private Sicherung mit vollständiger Historie
erhalten. Der für eine öffentliche Veröffentlichung vorgesehene Quellbaum wurde
als neues, unabhängiges Repository mit frischer Historie initialisiert. Zum
Zeitpunkt dieser Staging-Prüfung enthielt er weder alte Git-Objekte noch einen
Remote. Eine öffentliche Veröffentlichung bleibt davon getrennt und erfordert
eine ausdrückliche Freigabe.

## Lizenzinventar und Nachweisstand

Die Versionen folgen `tools/build.sh` und dem letzten lokalen Release-Build.
„Verifiziert“ bedeutet: Lizenzdatei in der installierten Distribution oder
eine verlinkte offizielle Quelle wurde festgestellt; es ist keine rechtliche
Bewertung der Gesamtdistribution.

| Bestandteil | Version/Quelle | Lizenzstand | Bekannte Verpflichtung / offene Frage |
|---|---|---|---|
| homeBert | Repository | MIT | Projekt-Lizenz und Copyright-Hinweis beim Quellcode beilegen. |
| HomeSpan | 2.1.8 | **VERIFIED:** MIT | MIT-Copyright- und Lizenzhinweis bei weitergegebenen Kopien/Binärbeilagen erhalten. |
| NimBLE-Arduino / Apache NimBLE | 2.5.1 | **VERIFIED:** Apache-2.0; NOTICE vorhanden | Apache-Lizenz und zutreffende NOTICE-/Copyright-Hinweise prüfen und beilegen. |
| TinyCrypt / micro-ecc in der NimBLE-Distribution | Teil der installierten Distribution | **PARTIALLY VERIFIED:** separate BSD-artige Hinweise | Exakte eingebundene Teile und Hinweise für das Image erfassen. |
| Arduino-ESP32 | 3.3.8 | **VERIFIED:** Paket-/Upstream-Angabe LGPL-2.1-or-later, weitere Komponentenlizenzen | Statisches Linken erfordert einen konkret geprüften LGPL-2.1-§6-Ansatz. |
| ESP-IDF | 5.5.4 laut Paketmetadaten | **PARTIALLY VERIFIED:** gemischte Komponentenlizenzen | Exakt gelinkte Komponenten, Notices und Quellen zuordnen. |
| Espressif Wi-Fi, Bluetooth, PHY, Koexistenz | `esp32-libs-3.3.8` | **PARTIALLY VERIFIED:** offizielle Upstream-Repositories nennen Apache-2.0 | Exakte Archive/Revisionen des installierten Pakets sind noch nicht vollständig zugeordnet. |
| GCC-Laufzeit und Newlib | Xtensa-Toolchain | **PARTIALLY VERIFIED:** lokale Lizenztexte vorhanden, einschließlich Runtime-/Newlib-Dateien | Tatsächlich gelinkte Laufzeitarchive, Ausnahmen und beizulegende Hinweise bestimmen. |
| FreeRTOS, lwIP, mbedTLS, NVS und weitere ESP-IDF-Anteile | Link-Map / ESP-IDF | **OPEN:** Bestandteile vorhanden | Komponentenbezogene Lizenz- und Notice-Inventur für das finale Image erstellen. |

Die offizielle Quellenliste und die detaillierten offenen Fragen stehen in
[Lizenzierung und Weitergabe von Firmware](licensing-and-redistribution.md).
`THIRD_PARTY_NOTICES.md` ist derzeit nur ein Ausgangspunkt und keine vollständige
Beilage für Firmware-Binärdateien.

## Veröffentlichungswege

### A. Nur Quellcode

Der einfachste durch die vorhandenen Nachweise gestützte Weg ist die
Veröffentlichung des eigenen MIT-lizenzierten Quellcodes **ohne** vorgebaute
Firmware und ohne mitgelieferte Drittanbieterarchive. Erfordert mindestens:

- `LICENSE`, Lizenzhinweise und einen korrekten Hinweis auf Drittanbieter;
- reproduzierbare Bauanleitung mit den gepinnten Versionen;
- keine realen Identitäts-, Pairing- oder Netzwerkinformationen in aktuellem
  Baum oder erreichbarer Historie;
- Überprüfung, ob verwendete Quellen/Beispiele selbst fremde Lizenztexte oder
  urheberrechtlich geschützte Inhalte enthalten.

Die Abwesenheit von Binärdateien reduziert die binärspezifischen
Weitergabepflichten, ersetzt aber weder Privacy-Bereinigung noch die Prüfung
von Quellcode-/Notice-Pflichten der übernommenen Bestandteile.

### B. Quellcode und Firmware-Binärdateien

Dieser Weg benötigt zusätzlich ein geprüftes Release-Paket mit mindestens:

- `homebert-full.bin`, `homebert-update.bin`, `SHA256SUMS` und
  versionsgebundener Build-Provenienz;
- vollständigen anwendbaren Lizenztexten, Copyrights und Notices;
- dokumentiertem, fachlich geprüften LGPL-2.1-§6-Compliance-Ansatz für die
  statisch gelinkten Arduino-ESP32-Anteile;
- einer komponentengenauen Zuordnung der tatsächlichen ESP-IDF/Espressif- und
  GCC/Newlib-Anteile zum Image;
- einer erneuten Privacy-/Security-Prüfung des finalen Pakets.

**Empfehlung:** Nur den Quellcode veröffentlichen; Herkunft und
Veröffentlichungsberechtigung der Hardware-Grafik sind dokumentiert.
Firmware-Binärdateien erst nach Abschluss der vollständigen
Komponenten-/Notice- und LGPL-Prüfung als Release-Assets anbieten.

## Projektpositionierung

homeBert 0.4.0 ist die erste öffentliche HomeKit-Edition und enthält nur die
vorhandene Standard-Firmware. Advanced und Home Assistant sind ausdrücklich
nicht Teil dieser Version. Das Projekt ist unabhängig entwickelt und weder von
Nubert noch von Apple unterstützt oder freigegeben. Es wurde in einer eigenen
Umgebung genutzt; daraus folgt keine allgemeine Stabilitäts-, Verfügbarkeits-
oder Kompatibilitätszusage.

## Herkunft der Bluetooth-Protokollinformationen

Die für homeBert benötigten Bluetooth-Protokollinformationen wurden durch
Beobachtung und Analyse des Kommunikationsverhaltens zwischen einem Nubert
nuXinema Receiver und der zugehörigen Steuerungs-App auf eigener Hardware
ermittelt. Dabei wurden weder Nubert-Firmware noch Anwendungs-Binärdateien
dekompiliert. homeBert ist eine eigenständige Implementierung zur Herstellung
der Interoperabilität und kein offizielles Produkt von Nubert.

Diese Tatsachenbeschreibung ist keine rechtliche Bewertung und ersetzt keine
Rechtsberatung. Im geprüften Quellbaum wurde kein kopierter Nubert-Quellcode
oder sonstiges konkret identifiziertes fremdes Protokollmaterial festgestellt.
