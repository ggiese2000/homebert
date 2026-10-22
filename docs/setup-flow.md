# Einrichtung und Wiederherstellung

homeBert speichert die Konfiguration im NVS-Namensraum `homebert`.
WLAN-Zugangsdaten sowie ausgewählte BLE-Adresse und -Typ werden weder
kompiliert noch eingecheckt; WLAN-Passwörter werden nicht protokolliert. Der
gerätespezifische achtstellige HomeKit-Einrichtungscode und die vierstellige
QR-Setup-ID werden deterministisch aus der ESP32-Basis-MAC abgeleitet. Sie
werden bei jedem Boot ausgegeben und benötigen keine NVS-Speicherung.

1. Ohne gültige WLAN-Konfiguration startet homeBert den temporären Access
   Point `homeBert Setup` mit einem lokalen, dreistufigen Assistenten. Die
   Adresse ist gewöhnlich `http://192.168.4.1/`.
2. Der Assistent sucht aktiv nach connectbaren Geräten mit dem von Nubert
   beobachteten Werbe-Service `0000a600-0000-1000-8000-00805f9b34fb`. Das ist
   ein Auswahlkriterium, keine Zusage der GATT-Protokollkompatibilität. Name,
   BLE-Adresse und Adresstyp des ausgewählten Eintrags bleiben zusammen
   erhalten. Auch ein namenloses passendes Gerät ist auswählbar. Vor dem Scan
   die Nubert-App vollständig schließen; sie kann die Steuerverbindung halten.
3. WLAN aus der optionalen Suche wählen oder SSID (auch verborgen) manuell
   eingeben, Passwort ergänzen und die Konfiguration speichern.
4. Der Access Point stoppt vor dem Stationsmodus. Die erste Verbindung erhält
   bis zu 60 Sekunden für Authentifizierung und DHCP.
5. Nach der ersten erfolgreichen Verbindung wird `wifi_verified` gespeichert;
   HomeSpan und die BLE-Verbindung starten.
6. Eine fehlgeschlagene erste Verbindung kehrt zum Setup zurück und bewahrt den
   gewählten Receiver. Spätere WLAN-Ausfälle lösen unbegrenzt Reconnects aus,
   öffnen aber niemals das Setup und löschen keine Paarungsdaten.

## HomeKit-Pairing wiederherstellen

Wenn homeBert aus Apple Home entfernt wurde, während das Gerät nicht
erreichbar war, können die HomeKit-Controllerdaten auf homeBert verbleiben.
Die native HomeSpan-Konsole ist der einzige serielle Eingabe-Consumer. Sie
zeigt beim HomeSpan-Start den Hinweis, dass der serielle Monitor Zeilenenden
senden muss. Mit GNU screen bei 115200 Baud daher einen Zeilenumbruch (LF)
senden; CRLF wird ebenfalls akzeptiert. Die Konsole selbst erzeugt keinen
eigenen Prompt oder Echo. `?` zeigt die vollständige HomeSpan-Hilfe samt
homeBert-Zusatzbefehlen.

- `@n`: zeigt den bekannten homeBert-/nuXinema-Status und fordert bei
  bestehender BLE-Verbindung eine lesende Statusaktualisierung an.
- `@p`: zeigt den deterministischen HomeKit-Code, die Setup-ID und den
  `X-HM://`-Payload für ein QR-Label, ohne Daten zu verändern.
- `s`: zeigt den nativen HomeSpan-Verbindungsstatus.
- `U`: entfernt ausschließlich die HomeKit-Controllerpaarungen. Der Befehl
  wird nie automatisch ausgeführt.

Nach `U` in Apple Home „Gerät hinzufügen“ wählen und den beim Boot oder mit
`@p` ausgegebenen Code eingeben. `U` bewahrt WLAN-Zugangsdaten,
BLE-Receiverauswahl, Firmware, HomeKit-Geräteidentität, Einrichtungscode und
HomeSpan-SRP-Verifikationsdaten. Der Befehl löscht absichtlich alle bestehenden
Apple-Home-Paarungen, führt jedoch keinen Werksreset aus.

Die Details und Testvektoren der deterministischen Ableitung stehen unter
[Deterministische HomeKit-Identität](homekit-identity.md).

Ein Werksreset löscht dagegen die lokale Konfiguration und HomeKit-Paarungsdaten
und startet danach erneut das Setup.

## Serielle Wartung und destruktive HomeSpan-Befehle

Die native HomeSpan-Konsole bleibt für Diagnose und Wartung aktiviert. HomeSpan
2.1.8 führt ihre eingebauten Löschbefehle (`U`, `X`, `V`, `H`, `F`, `E`) direkt
aus und bietet dafür keinen Callback oder Bestätigungs-Hook. Deshalb kann
homeBert diese Befehle nicht einzeln mit einer physischen Tasterbestätigung
schützen, ohne die native Konsole abzuschalten oder die Drittanbieterbibliothek
zu ändern. Niemals Logausgaben oder mehrzeiligen Text in diese Konsole einfügen.
Der physische Werksreset per mindestens 10 Sekunden gehaltenem ATOM-Lite-Taster
bleibt der dokumentierte absichtliche lokale Resetpfad. Details zu LED,
Fronttaster und dem separaten Hardware-Reset stehen unter
[LED und Taster](LED_AND_BUTTON.md).
