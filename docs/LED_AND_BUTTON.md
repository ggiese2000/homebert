# LED und Taster – Kurzreferenz

Der ATOM Lite wird über USB-C mit Strom versorgt. Der große Fronttaster
(`GPIO39`) ist der homeBert-Bedientaster; der kleine seitliche **Reset**-Taster
ist ein Hardware-Reset des ESP32. Er startet nur die Firmware neu. Er ist
**kein** Werksreset und löscht weder WLAN-Zugangsdaten, BLE-Auswahl,
HomeKit-Paarungen noch andere homeBert-Einstellungen.

## Fronttaster

| Aktion | Wirkung |
|---|---|
| Einmal kurz drücken | Zeigt den Verbindungszustand fünf Sekunden auf der LED. Die Anzeige erscheint nach Ablauf des 350-ms-Doppelklickfensters. |
| Zweimal kurz drücken | Trennt eine bestehende BLE-Verbindung und fordert den normalen BLE-Wiederverbindungsweg an. |
| Mindestens 10 Sekunden gedrückt halten | Führt sofort, ohne Loslassen, den Werksreset aus. Die LED leuchtet etwa eine Sekunde rot. |

Ein Werksreset löscht WLAN-Zugangsdaten, BLE-Auswahl, HomeKit-Paarungen und
weitere homeBert-Einstellungen. Firmware und die werkseitige ESP32-Identität
bleiben erhalten. Ein Halten unter zehn Sekunden löst keinen Neustart aus.

Der große Fronttaster und der kleine seitliche Reset-Taster haben damit bewusst
unterschiedliche Wirkungen: Nur der zehn Sekunden gehaltene **Fronttaster**
löscht Konfiguration.

## LED

Die LED verwendet eine feste Priorität: WLAN-Warnungen und Setup haben Vorrang
vor Start-, Verbindungs-, Status- und BLE-Befehlsanzeigen. Es gibt in der
aktuellen Firmware keinen konfigurierbaren Dauer-/Aus-Modus.
Eine abweichende LED-Logik ist nur durch Änderung und Neubau der Firmware
möglich; die Einrichtung bietet dafür keine Benutzereinstellung.

| Zustand | Anzeige |
|---|---|
| Start | Weiß für etwa 0,5 Sekunden |
| Setup-Hotspot aktiv | Gelb blinkend, etwa 0,5 s an / 0,5 s aus |
| Erste WLAN-Prüfung fehlgeschlagen | Gelb/Rot abwechselnd, je etwa 0,5 s |
| WLAN länger als 30 s nicht verfügbar (nach zuvor erfolgreicher Einrichtung) | Rot blinkend, etwa 0,5 s an / 0,5 s aus |
| Erster BLE-Verbindungsaufbau nach normalem Boot | Blau blinkend, mindestens zwei Sekunden; danach bis zum erfolgreichen Verbindungsaufbau weiter blinkend |
| WLAN und nuXinema-BLE verbunden | Grün für fünf Sekunden nach erfolgreicher Verbindung, danach aus |
| Stabile verbundene Normalfunktion | Aus |
| BLE-Wiederverbindung | Blau blinkend während des Aufbaus; bei Erfolg danach Grün für fünf Sekunden |
| Erfolgreich übertragener ändernder nuXinema-Befehl | Blau für etwa 0,1 s, sofern keine Warnung aktiv ist |
| Kurzstatus nach Fronttasterdruck | Fünf Sekunden: Grün (WLAN+BLE), Blau (nur WLAN) oder Rot (kein WLAN) |
| Werksreset | Rot dauerhaft für etwa eine Sekunde |

Der gelb blinkende Setup-Hotspot wurde auf Hardware beobachtet. Die genaue
Startsequenz, die WLAN-Fehleranzeigen und die Zeitwerte sind mit automatisierten
Zustandstests abgedeckt und benötigen noch eine abschließende Hardwareprüfung.
Die Zeitwerte sind Firmware-Sollwerte; die tatsächliche Sichtbarkeit hängt von
der Ausführung der Hauptschleife und der LED-Hardware ab.
