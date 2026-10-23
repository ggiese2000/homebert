# Firmware aus Quellcode bauen

Diese Anleitung beschreibt die lokale, versionsgebundene Erzeugung der
homeBert-0.4.0-Images aus einem Quellcode-Checkout. Sie veröffentlicht keine
Firmware-Binärdateien und ersetzt nicht die separate Prüfung für deren
öffentliche Weitergabe.

Der Ablauf wurde mit den unten genannten, bereits lokal installierten Versionen
erfolgreich gebaut. Ein vollständig frischer Checkout benötigt beim ersten
Lauf Zugriff auf die offiziellen Arduino- und Espressif-Paketregister. Der
vollständige Installationsweg auf einem frisch eingerichteten Computer wurde
noch nicht separat getestet.

## Voraussetzungen

- Git und eine POSIX-Shell (getestet unter Linux; macOS benötigt äquivalente
  Werkzeuge wie `bash`, `find`, `cmp` und `sha256sum` oder passende Varianten).
- [Arduino CLI 1.3.1](https://github.com/arduino/arduino-cli/releases/tag/1.3.1).
  Eine Arduino IDE ist nicht erforderlich.
- Internetzugang beim ersten Build für die offiziellen Arduino- und Espressif-
  Paketindizes sowie die unten genannten, gepinnten Abhängigkeiten.
- Genügend lokaler Speicher für Arduino-Pakete und Build-Ausgaben.

Der Build verwendet ausschließlich ein lokales Arduino-Verzeichnis im
Repository (`.arduino/`). Es wird durch Git ignoriert. Systemweite
Arduino-Installationen werden nicht verändert.

## Gepinnte Build-Umgebung

| Bestandteil | Version / Einstellung |
|---|---|
| Arduino CLI | `1.3.1` |
| Arduino-ESP32 | `3.3.8` |
| HomeSpan | `2.1.8` |
| NimBLE-Arduino | `2.5.1` |
| FQBN | `esp32:esp32:esp32:PartitionScheme=min_spiffs` |
| Zielhardware | M5Stack ATOM Lite / klassischer ESP32, 4 MB Flash |

Die Konfiguration steht in `arduino-cli.yaml`; die Abhängigkeiten und Versionen
werden in `tools/build.sh` geprüft und installiert. Arduino CLI muss exakt
Version 1.3.1 melden. Ein anderer Wert wird absichtlich abgewiesen, damit ein
Build nicht unbemerkt mit einer nicht geprüften Toolversion erfolgt.

## Build ausführen

1. Repository klonen und in das Verzeichnis wechseln.
2. Arduino CLI 1.3.1 entpacken, ohne sie systemweit installieren zu müssen.
3. Den Release-Build mit dem Pfad zur CLI starten:

   ```bash
   ARDUINO_CLI=/absoluter/pfad/zu/arduino-cli ./tools/release.sh
   ```

`tools/release.sh` verlangt einen sauberen Git-Arbeitsbaum. Es ruft den
zentralen Compiler-Ablauf `tools/build.sh` auf, der die gepinnten Pakete in
`.arduino/` installiert, die Firmware baut und Versions- sowie Commit-Metadaten
einbettet.

Nach erfolgreichem Build liegen die lokalen, ignorierten Ergebnisse unter
`build/release/`:

| Datei | Inhalt |
|---|---|
| `homebert-full.bin` | Bootloader, Partitionstabelle, OTA-Metadaten und Anwendung für die Erstinstallation |
| `homebert-update.bin` | Nur die Anwendung für eine kompatible bestehende Installation |
| `SHA256SUMS` | SHA-256-Prüfsummen beider Images |
| `BUILD_INFO` | Version, Git-Commit, FQBN, Größen und Prüfsummen |

Das Skript prüft die Image-Struktur mit `esptool`, vergleicht das Update-Image
mit dem Anwendungsteil des Vollimages und validiert die erzeugten Prüfsummen.

## Flash-Offsets und Partitionsvoraussetzungen

Das vollständige Image wird an `0x0` geschrieben:

```bash
esptool --chip esp32 --port <PORT> --baud 115200 write-flash -z 0x0 build/release/homebert-full.bin
```

Das Update-Image wird ausschließlich an `0x10000` geschrieben:

```bash
esptool --chip esp32 --port <PORT> --baud 115200 write-flash -z 0x10000 build/release/homebert-update.bin
```

Das Update setzt das vorhandene `min_spiffs`-Partitionslayout voraus. Die
Tabelle `firmware/partitions.csv` definiert zwei OTA-Anwendungsbereiche zu je
`0x1e0000` Byte. Das Vollimage enthält keine NVS-Daten; eine Erstinstallation
kann daher gespeicherte Konfiguration im beschriebenen Flash-Bereich ersetzen.

## Häufige Fehler

| Meldung / Situation | Ursache und Abhilfe |
|---|---|
| `Arduino CLI was not found` | Arduino CLI 1.3.1 installieren oder den absoluten Pfad über `ARDUINO_CLI` setzen. |
| `Arduino CLI 1.3.1 is required` | Die angegebene CLI-Version verwenden; andere Versionen sind für 0.4.0 nicht geprüft. |
| Paketindex oder Paketinstallation schlägt fehl | Netzwerkzugang und Proxy-Konfiguration prüfen; anschließend denselben Befehl wiederholen. Bereits installierte gepinnte Pakete werden wiederverwendet. |
| `release generation requires a clean Git worktree` | Änderungen committen, staschen oder in einem separaten sauberen Checkout bauen. |
| Update startet nicht | Prüfen, dass die vorhandene Installation das erwartete `min_spiffs`-Partitionslayout und `app0` bei `0x10000` verwendet; andernfalls das Vollimage nach gesicherter Konfiguration verwenden. |

## Lokaler Build ist keine Binärfreigabe

Die lokale Erzeugung der Images belegt die technische Reproduzierbarkeit. Sie
erteilt keine Freigabe, die erzeugten Binärdateien öffentlich weiterzugeben.
Die dafür noch offenen Lizenz-, Notice- und LGPL-Fragen stehen unter
[Lizenzierung und Weitergabe von Firmware](licensing-and-redistribution.md).
