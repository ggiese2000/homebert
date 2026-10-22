# Lizenzierung und Weitergabe von Firmware

## Zweck und aktueller Status

Dieses Dokument hält den Stand der technischen Lizenzrecherche für homeBert fest. Es ist kein Rechtsgutachten und keine Erklärung, dass die Weitergabe der Firmware lizenzkonform ist.

**Aktueller Status:** Der bereinigte Quellcode wird für eine spätere
Veröffentlichung vorbereitet. `homebert-full.bin` und `homebert-update.bin`
werden nicht veröffentlicht.

| Bereich | Bewertung |
|---|---|
| Private Entwicklung | **ASSESSMENT: CONTINUE** |
| Öffentliche Quellcode-Veröffentlichung | **READY WITH LIMITATIONS:** Privacy- und Notice-Prüfung des frischen Staging-Baums erforderlich |
| Öffentliche Firmware-Binärdistribution | **INCOMPLETE / NO-GO** |

Die vorherige Prüfung ergab unvollständige Drittanbieterhinweise, eine nicht vollständig belegte Zuordnung vorcompilierter Espressif-Archive sowie offene Fragen zu statischem Linken unter LGPL und zu GCC/Newlib-Laufzeitanteilen. Spätere Recherche reduziert die Unsicherheit für Wi-Fi, Bluetooth und PHY, ersetzt aber keine vollständige Paket- und Komponenten-Inventur.

## Verwendete Firmware-Komponenten und Versionen

**VERIFIED:** Die folgenden Angaben folgen der eingecheckten [Build-Konfiguration](../tools/build.sh), dem [Release-Ablauf](../tools/release.sh) und der lokalen Paketmetadatei `.arduino/data/packages/esp32/tools/esp32-libs/3.3.8/versions.txt`.

| Bestandteil | Stand | Nachweis / Rolle |
|---|---:|---|
| Zielhardware | M5Stack ATOM Lite, klassischer ESP32, 4 MB Flash | Projektkonfiguration |
| Arduino-ESP32 | 3.3.8 | `tools/build.sh` |
| ESP-IDF | v5.5.4 | `esp32-libs`-Metadaten |
| HomeSpan | 2.1.8 | `tools/build.sh` |
| NimBLE-Arduino | 2.5.1 | `tools/build.sh` |
| Vollimage | `homebert-full.bin` | `tools/release.sh`; enthält Bootloader, Partitionstabelle, `boot_app0.bin` und Anwendung |
| Update-Image | `homebert-update.bin` | `tools/release.sh`; enthält nur die Anwendung für Offset `0x10000` |

**VERIFIED:** Der Link-Map der erzeugten Anwendung weist neben HomeSpan und NimBLE-Arduino unter anderem Arduino-ESP32-/ESP-IDF-Code, Wi-Fi- und Bluetooth-Stacks, PHY- und Koexistenzbibliotheken, FreeRTOS, lwIP, mbedTLS, NVS sowie C/C++-Laufzeitarchive nach. Arduino CLI und esptool sind dagegen Build-Werkzeuge und nicht Bestandteil der Flash-Images. Statisch gelinkte GCC-/Newlib-Laufzeitarchive sind dagegen Bestandteil der Anwendung und müssen separat betrachtet werden.

## Bereits identifizierte Lizenzbedingungen

### HomeSpan 2.1.8

**VERIFIED:** Die lokal installierte HomeSpan-Version enthält eine MIT-Lizenz mit Copyright `2020-2025 Gregg E. Berman`. Bei einer Binärdistribution müssen Copyright- und MIT-Lizenzhinweis beigefügt werden.

### NimBLE-Arduino 2.5.1 und Apache NimBLE

**VERIFIED:** Die lokal installierte NimBLE-Arduino-Distribution enthält die Apache License 2.0 und eine `NOTICE`-Datei. Die anwendbaren Lizenztexte und NOTICE-Informationen müssen einer Binärdistribution beigefügt werden.

**VERIFIED:** Die eingebundene TinyCrypt-/micro-ecc-Komponente hat eigene BSD-artige Hinweise für Binärweitergabe. Diese Hinweise sind getrennt von Apache-2.0 zu prüfen und beizufügen.

### Arduino-ESP32 3.3.8

**VERIFIED:** Die offizielle Arduino-ESP32-Paketmetadatei für 3.3.8 nennt `LGPL-2.1-or-later`. Die Lizenz ist unter [Arduino-ESP32 3.3.8](https://github.com/espressif/arduino-esp32/tree/3.3.8) und als [LGPL 2.1](https://www.gnu.org/licenses/old-licenses/lgpl-2.1.html) dokumentiert.

**OPEN:** Für die statisch gelinkte Firmware muss ein nachvollziehbarer LGPL-2.1-Abschnitt-6-Compliance-Ansatz bestimmt und umgesetzt werden. Die Veröffentlichung des Quellcodes und reproduzierbare Builds sind dafür relevant, beweisen aber für sich allein keine Erfüllung der Anforderungen. Insbesondere sind die Anforderungen an Lizenzhinweis, Zugang zum verwendeten Bibliotheksquellcode und die Möglichkeit zum Relinken anhand der tatsächlich gewählten Bereitstellungsform zu bewerten.

### Espressif-Binärbibliotheken

**PARTIALLY VERIFIED:** Die folgenden offiziellen Espressif-Repositories bezeichnen ihre jeweiligen vorcompilierten Bibliotheken als Apache-2.0 und beschreiben sie als ESP-IDF-Teil:

- [ESP32 Wi-Fi libraries](https://github.com/espressif/esp32-wifi-lib)
- [ESP32 Bluetooth libraries](https://github.com/espressif/esp32-bt-lib)
- [ESP PHY libraries](https://github.com/espressif/esp-phy-lib)

**OPEN:** Es ist noch nicht vollständig nachgewiesen, welche konkreten Archive und Revisionen aus dem installierten Paket `esp32-libs-3.3.8` genau diesen Upstream-Deklarationen entsprechen. Die Paketmetadaten nennen ESP-IDF v5.5.4, enthalten aber keine vollständige, release-taugliche Lizenz- oder NOTICE-Inventur für alle gelinkten Archive. Die allgemeine Apache-2.0-Deklaration allein genügt deshalb nicht als vollständiger Nachweis für das veröffentlichte Binärpaket.

### ESP-IDF v5.5.4

**PARTIALLY VERIFIED:** ESP-IDF verwendet Apache-2.0 für eigenen Code, enthält jedoch Komponenten Dritter unter unterschiedlichen Lizenzen. Die offizielle, versionsspezifische Referenz ist [ESP-IDF v5.5.4 COPYRIGHT](https://github.com/espressif/esp-idf/blob/v5.5.4/docs/en/COPYRIGHT.rst).

**OPEN:** Für die konkret gelinkte Firmware ist weiterhin eine komponentenbezogene Inventur mit den jeweils anwendbaren Lizenztexten, Copyright- und NOTICE-Hinweisen erforderlich.

### GCC und Newlib

**VERIFIED:** Der Link-Map weist C/C++-Laufzeitarchive des Xtensa-Toolchains und Newlib-Anteile aus; diese sind nicht nur Build-Werkzeuge.

**OPEN:** Die exakten Lizenzen, Laufzeit-Ausnahmen, Copyright-Hinweise und Weitergabepflichten der tatsächlich verwendeten Toolchain-Archive sind anhand der installierten Toolchain oder einer dazugehörigen offiziellen Lizenzdistribution noch zu erfassen. Sie werden in diesem Dokument nicht erraten.

## Offizielle Quellen und Nachweise

Die folgenden Quellen wurden als Lizenznachweise erfasst. Eine Quelle belegt nicht automatisch, dass jedes gleichnamige Archiv im erzeugten Image aus genau dieser Quelle stammt.

| Quelle | Aussagebereich | Status |
|---|---|---|
| [HomeSpan-Lizenz](https://github.com/HomeSpan/HomeSpan/blob/2.1.8/LICENSE) | MIT-Lizenz von HomeSpan | **VERIFIED** zusätzlich lokal geprüft |
| [NimBLE-Arduino 2.5.1](https://github.com/h2zero/NimBLE-Arduino/tree/2.5.1) | Apache-2.0, NOTICE und eingebundene Bestandteile | **VERIFIED** zusätzlich lokal geprüft |
| [Arduino-ESP32 3.3.8](https://github.com/espressif/arduino-esp32/tree/3.3.8) | Paketversion und LGPL-2.1-or-later | **VERIFIED** |
| [LGPL 2.1, Abschnitt 6](https://www.gnu.org/licenses/old-licenses/lgpl-2.1.html) | Bedingungen für verlinkte Werke | **VERIFIED** Quelle; Anwendung auf homeBert ist noch zu bewerten |
| [Espressif Wi-Fi](https://github.com/espressif/esp32-wifi-lib) | Apache-2.0-Deklaration für Wi-Fi-Binärbibliotheken | **PARTIALLY VERIFIED** |
| [Espressif Bluetooth](https://github.com/espressif/esp32-bt-lib) | Apache-2.0-Deklaration für Bluetooth-Binärbibliotheken | **PARTIALLY VERIFIED** |
| [Espressif PHY](https://github.com/espressif/esp-phy-lib) | Apache-2.0-Deklaration für PHY-Binärbibliotheken | **PARTIALLY VERIFIED** |
| [ESP-IDF v5.5.4 COPYRIGHT](https://github.com/espressif/esp-idf/blob/v5.5.4/docs/en/COPYRIGHT.rst) | ESP-IDF- und Drittanbieter-Lizenzübersicht | **PARTIALLY VERIFIED**; Zuordnung zum Image offen |

## Offene Fragen und mögliche Risiken

- **OPEN:** Vollständige Archiv-zu-Upstream-Revision-Zuordnung für die gelinkten Espressif-Binärbibliotheken, insbesondere Wi-Fi, Bluetooth, PHY und Koexistenz.
- **OPEN:** Vollständige Lizenz-, Copyright- und NOTICE-Inventur der aus `esp32-libs-3.3.8` tatsächlich in die Anwendung gelinkten Komponenten.
- **OPEN:** Lizenztexte und Laufzeitausnahmen für die gelinkte GCC-/Newlib-Laufzeitumgebung.
- **OPEN:** Konkreter, prüfbarer LGPL-2.1-Abschnitt-6-Ansatz für die statisch gelinkte Arduino-ESP32-Komponente.
- **ASSESSMENT:** Ohne diese Nachweise kann `THIRD_PARTY_NOTICES.md` nicht als vollständige Begleitdokumentation für Release-Binärdateien gelten.

## Anforderungen vor einer öffentlichen Veröffentlichung

Vor der Veröffentlichung von `homebert-full.bin` oder `homebert-update.bin` müssen alle Punkte erledigt und dokumentiert sein:

- [ ] Exakte Herkunft, Version und Lizenz jedes tatsächlich verteilten Komponentenanteils feststellen.
- [ ] `THIRD_PARTY_NOTICES.md` anhand dieser Inventur vervollständigen.
- [ ] Alle anwendbaren Lizenztexte, Copyright- und NOTICE-Hinweise in einem Release-Begleitpaket bereitstellen.
- [ ] Gültigen LGPL-2.1-Abschnitt-6-Compliance-Ansatz auswählen, umsetzen und prüfen lassen.
- [ ] Lizenzen und Ausnahmen der tatsächlich gelinkten GCC-/Newlib-Laufzeitarchive verifizieren.
- [ ] Finale Release-Struktur mit beiden Images, `SHA256SUMS`, Notices und Lizenztexten validieren.
- [ ] Vor der Veröffentlichung erneut Privacy-/Security-Prüfung durchführen.

## Konkrete nächste Schritte

1. Bei Espressif eine versionsgebundene Lizenz-/NOTICE-Inventur oder SBOM für `esp32-libs-3.3.8` und ESP-IDF v5.5.4 beschaffen beziehungsweise aus offiziellen Quell- und Release-Artefakten reproduzierbar erzeugen.
2. Die Link-Map in eine nachvollziehbare Komponentenliste überführen und die Archive den offiziellen Upstream-Revisionen zuordnen.
3. Die Lizenzdateien der verwendeten Xtensa-GCC-/Newlib-Toolchain erfassen, einschließlich eventueller Runtime Exceptions.
4. Den LGPL-Compliance-Weg fachlich prüfen lassen; erst anschließend die konkrete Bereitstellung von Quellen, Objekten oder Relink-Material festlegen.
5. `THIRD_PARTY_NOTICES.md` und ein schlankes `licenses/`-Verzeichnis erst nach Abschluss dieser Inventur aktualisieren.
6. Abschließend das reale Release-Paket prüfen; bis dahin keine Binärdateien als GitHub-Release-Assets veröffentlichen.
