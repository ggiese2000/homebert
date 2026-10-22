# Release Notes – 0.4.0 (Entwurf der ersten öffentlichen HomeKit-Edition)

## Für wen?

Diese Firmware richtet sich an Besitzer eines M5Stack ATOM Lite und eines
Nubert nuXinema preAV. Andere Nubert-Modelle sind nicht als kompatibel
bestätigt. homeBert ist ein unabhängiges Community-Projekt; Stabilität,
Verfügbarkeit und Kompatibilität werden nicht zugesichert.

## Wichtigste Änderungen seit dem frühen Entwicklungsstand

- Neuer lokaler Einrichtungsassistent für Bluetooth-Gerät und WLAN.
- Native Apple-Home-Steuerung für Power ON/Standby, Eingänge und Lautstärke.
- Zustandsabgleich über BLE-Rückmeldungen statt nur über zuletzt gesendete
  Befehle; ausstehende Volume- und Power-Anfragen werden gezielt abgeglichen.
- Automatische Surround-Zuordnung nach bestätigter HomeKit-Eingangswahl.
- Deterministischer HomeKit-Setup-Code, Setup-ID und QR-Code pro Gerät.
- LED-Start-/Verbindungsanzeige und klare Tastergesten.
- Reproduzierbare lokale Images mit Prüfsummen und Provenienzmetadaten.

## Installation und Upgrade

`homebert-full.bin` wird bei einer Erstinstallation an `0x0` geflasht.
`homebert-update.bin` aktualisiert eine vorhandene `app0`-Installation an
`0x10000` und erhält die NVS-Konfiguration. Genaue Befehle stehen in der
[README](../README.md#firmware-installieren-und-aktualisieren).

## Bekannte Einschränkungen

- Nubert-App und homeBert können praktisch um die BLE-Steuerverbindung
  konkurrieren.
- Die BLE-Recovery nach vollständigem Stromverlust des Receivers benötigt noch
  zusätzliche Hardwarevalidierung.
- Die Firmware ist nur für den genannten ATOM Lite und nuXinema preAV getestet.
- Advanced und Home Assistant sind nicht Teil von 0.4.0.
- Dieses Dokument ist kein Veröffentlichungsnachweis: Die öffentliche
  Binärweitergabe bleibt bis zum Abschluss der Lizenzinventur gesperrt.
