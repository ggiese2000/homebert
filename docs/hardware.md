# M5Stack ATOM Lite

| Merkmal | Wert |
|---|---|
| SoC | ESP32-PICO-D4, Dual-Core LX6, bis 240 MHz |
| SRAM | 520 KB intern |
| Flash | 4 MB SPI-Flash, kein PSRAM |
| Großer Fronttaster | GPIO39, aktiv niedrig |
| RGB-LED | SK6812 an GPIO27, GRB-Reihenfolge |
| Kleiner seitlicher Reset-Taster | Hardware-Reset: startet den ESP32 neu, kein homeBert-Werksreset |
| Infrarot | GPIO12, unbenutzt |

GPIO39 ist nur ein Eingang; die Firmware verwendet deshalb `INPUT` ohne
internen Pull-up. Die verbindliche Bedien- und LED-Referenz steht unter
[LED und Taster](LED_AND_BUTTON.md). Die Grafik in der
[README](../README.md) hilft bei der physischen Zuordnung,
ersetzt aber nicht die aktuelle LED-Zustandstabelle.
