# Phase 2: native HomeKit-Integration

## Zubehörmodell

Ein HomeKit-Zubehör enthält einen nativen Television-Service mit neun
verknüpften InputSource-Services sowie einen verknüpften TelevisionSpeaker für
die relativen Lautstärketasten der iOS-Fernbedienung. `Active` steht für den
bestätigten Receiver-Powerzustand und `ActiveIdentifier` für den bestätigten
Eingang. Die stabilen HomeKit-Eingangskennungen behalten ihre BLE-Wertezuordnung
(`identifier - 1`); `DisplayOrder` zeigt dagegen die Reihenfolge der
physischen Fernbedienung: eARC, HDMI 1, HDMI 2, HDMI 3, Coax, Bluetooth, AUX,
Optical, Port. Diese Ordnung folgt der von Nubert dokumentierten Gruppierung
der Fernbedienung: HDMI, Coax, Bluetooth, AUX, Opto, Port.

| Anzeige-Reihenfolge | HomeKit-Identifier | BLE-Eingangswert |
|---|---:|---:|
| eARC | 1 | `00` |
| HDMI 1 | 2 | `01` |
| HDMI 2 | 3 | `02` |
| HDMI 3 | 4 | `03` |
| Coax | 7 | `06` |
| Bluetooth | 6 | `05` |
| AUX | 5 | `04` |
| Optical | 8 | `07` |
| Port | 9 | `08` |

Die Reihenfolge der Nubert-Fernbedienung ist in der
[nuXinema-preAV-Bedienungsanleitung](https://www.nubert.de/down/ab/e2/5a/1690272485/nuxinema-preav-bedienungsanleitung.pdf)
dokumentiert. Die Kennungen bleiben bewusst unverändert; jede wird über
`identifier - 1` auf ihren ursprünglichen BLE-Wert abgebildet. AUX ist auf
Hardware geprüft. Die übrigen Eingabebefehle behalten ihre beobachtete
Protokollzuordnung und benötigen eine individuelle Hardwarevalidierung, bevor
sie als hardwaregeprüft bezeichnet werden.

Der TelevisionSpeaker-Service besitzt keine lesbare Charakteristik für eine
absolute Lautstärke. Dasselbe Zubehör ergänzt deshalb ein kleines kompatibles
Steuerelement:

- `Volume` ist ein Fan-`RotationSpeed`-Steuerelement, das direkt auf den
  Rohwert `0x00`–`0x64` des Receivers abgebildet wird (dB = Rohwert - 100).
  Fan-`Active` wird ignoriert: Es ändert weder die Receiver-Power noch sendet
  es einen Lautstärkebefehl.

Der frühere LightBulb-Service wurde durch einen Fan-Service ersetzt. Die
Firmware bewahrt bestehende HomeKit-Paarungen; nach dem Neustart sollte Apple
Home die Zubehördatenbank aktualisieren. Falls dort weiterhin die frühere
Licht-Kachel erscheint, homeBert in Apple Home entfernen und erneut
hinzufügen. Das ist eine Migration des Apple-Home-Service-Caches, kein
Firmware-Werksreset; WLAN- und Receiver-Konfiguration bleiben erhalten.

Es gibt kein separates Surround-Steuerelement. Eine ausdrückliche
HomeKit-Eingangsänderung reiht den passenden Surround-Befehl erst ein, nachdem
der Receiver diesen Eingang bestätigt hat: Auto (`05`) für eARC und HDMI 1–3,
Stereo (`00`) für alle übrigen Eingänge. Fernbedienungs- und physische
Eingangs-/Surround-Änderungen lösen nie automatische Surround-Befehle aus.

## Zustands- und Befehlsverarbeitung

Das zentrale Zustandsmodell speichert den BLE-Verbindungszustand sowie gültige
Power-, Lautstärke-, Eingangs- und Surround-Werte. Es wird aus einzelnen
Drei-Byte-Datensätzen und gebündelten `86`-TLV-Antworten aktualisiert. Bei
jeder GATT-Verbindung reiht die Firmware nach der Benachrichtigungsanmeldung
die Haupt- und Sound-Einstellungsstatusabfragen ein.

BLE-Benachrichtigungen rufen HomeSpan nie direkt auf. Sie markieren den Zustand
als geändert; die Hauptschleife veröffentlicht nur bestätigte, gültige Werte an
HomeKit. Eine Trennung löscht alle Gültigkeitsflags und setzt Power auf
UNKNOWN. Zwischengespeicherte HomeKit-Werte werden während einer Trennung nicht
als bestätigter Zustand erneut veröffentlicht.

HomeKit-Callbacks reihen nur einen BLE-Write ein; sie rufen niemals `setVal()`
auf, während HomeSpan diese Charakteristik verarbeitet. Die Hauptschleife
sendet den Write und reiht beide Statusabfragen 500 ms später ein. Ein
erfolgreicher GATT-Write gilt nicht als Bestätigung. Für Lautstärke und Power
gilt zusätzlich eine begrenzte,
flüchtige Pending-Reconciliation: Wird ein Befehl in die Queue aufgenommen,
akzeptiert HomeSpan die Benutzeranfrage zunächst. Veraltete Rückmeldungen
überschreiben den angeforderten Wert nicht; erst eine passende bestätigte
Empfänger-Rückmeldung, Fehler, Disconnect oder Timeout beendet den Pending-
Zustand. Dadurch wird weder ein GATT-Write noch das Absenden selbst mit einer
Gerätebestätigung verwechselt. Es gibt kein periodisches BLE-Polling.

Sendet Apple Home während eines akzeptierten HomeKit-Einschaltvorgangs eine
Eingangsanfrage, hält homeBert diese höchstens zehn Sekunden zurück. Es sendet
den Eingang erst nach bestätigtem ON und behält danach die bestehende Sequenz
aus bestätigtem Eingang und Surround-Befehl bei. Die zurückgestellte Anfrage
wird bei Timeout, Trennung, UNKNOWN-Zustand oder einer HomeKit-Standby-Anfrage
abgebrochen und nach einem späteren, unabhängigen Einschalten nie erneut
gesendet.

## Nachweise zu BLE-Befehlen

| Function | Status | Write | Phase 2 use |
|---|---|---|---|
| Power ON | `1E 01 00` | bestätigt `1F 01 00` | Nur aus bestätigt STANDBY aktiviert |
| Power OFF / STANDBY | `1E 01 01` | hardwaregeprüft `1F 01 01` | Nur aus bestätigt ON aktiviert |
| Lautstärke | `0A 01 value` | hardwaregeprüft `0B 01 value` | Aus bestätigt ON aktiviert |
| Eingang | `0E 01 value` | `0F 01 value` | Aktiviert; AUX (`04`) wurde einzeln geprüft |
| Surround | `93 01 value` | `94 01 value` | Automatisches Stereo (`00`) und Auto (`05`) nach HomeKit-Eingangsbestätigung |

Die Hauptstatusabfrage und die Sound-Einstellungsabfrage sind bestätigte,
aufgezeichnete reine Leseanfragen. `1F 01 01` wurde durch Write-Antwort,
Benachrichtigung und anschließende Statusabfrage auf Hardware geprüft.
Lautstärkebefehle sowie die automatischen Surround-Befehle Stereo
(`94 01 00`) und Auto (`94 01 05`) wurden anschließend am Zielreceiver
verifiziert. Die AUX-Eingangswahl wurde einzeln getestet; die übrigen
Eingangswerte sind beobachtete Statuszuordnungen mit demselben Befehlsformat
und bleiben Kandidaten für eine Hardwarevalidierung pro Eingang.

## Sicherheitsregeln

Eingangs-, Lautstärke- und Surround-Writes erfordern einen bestätigten
ON-Zustand. Power ON ist die einzige Ausnahme und erfordert bestätigt STANDBY;
es wird nie aus einer BLE-Verbindung oder einem erfolgreichen Write abgeleitet.
HomeKit Power OFF sendet das geprüfte `1F 01 01` nur im ON-Zustand und wartet
auf eine bestätigte `1E 01 01`-Benachrichtigung oder Aktualisierung. Ein
eingereihter Befehl verhindert weitere Befehle, bis er versucht und eine
gezielte Aktualisierung eingereiht wurde.

## Hardwarevalidierung

1. Mit 115200 Baud flashen, WLAN einrichten und den Receiver auswählen.
2. Den HomeSpan-Start bestätigen und `homeBert` in Apple Home mit dem
   deterministischen achtstelligen Einrichtungscode oder QR-Payload aus der
   seriellen Ausgabe koppeln. Beide Werte werden aus der ESP32-Basis-MAC
   abgeleitet; siehe [Deterministische HomeKit-Identität](homekit-identity.md).
3. GATT-Anmeldung, beide Statusabfrage-Write-Ergebnisse sowie Power-,
   Lautstärke-, Eingangs- und Surround-Logs prüfen. Die zugehörigen
   HomeKit-Steuerelemente erst nach der jeweiligen BLE-Benachrichtigung
   bestätigen.
4. Power, Lautstärke, Eingang und Surround über die Nubert-Fernbedienung
   ändern und prüfen, dass HomeKit ohne eigenen HomeKit-Befehl aktualisiert
   wird.
5. Aus HomeKit Power ON und während des Übergangs einen Eingang anfordern.
   `HomeKit input deferred` prüfen, anschließend einen Eingangs-Write erst
   nach `1E 01 00`. Der zugeordnete Surround-Write muss dem bestätigten Eingang
   folgen.
6. Die Anzeigeordnung eARC, HDMI 1, HDMI 2, HDMI 3, Coax, Bluetooth, AUX,
   Optical, Port prüfen. AUX auf Hardware prüfen; die übrigen Eingangs-Writes
   bleiben protokollabgeleitete Kandidaten für individuelle Hardwaretests.
7. Prüfen, dass Fan Speed die Receiver-Lautstärke steuert und
   Receiver-Lautstärkeänderungen Fan Speed aktualisieren. Fan Active ändern
   und bestätigen, dass kein BLE-Write und keine Poweränderung entsteht.
   Prüfen, dass Apples Aktion „Alle Lichter aus“ die Lautstärke nicht mehr
   einschließt.
8. Im STANDBY- oder UNKNOWN-Zustand Lautstärke-, Eingangs- und
   Surround-Änderungen versuchen und prüfen, dass `Nubert device command
   blocked` ohne GATT-Write protokolliert wird.
9. In HomeKit das Television-Zubehör im ON-Zustand ausschalten. `Power
   STANDBY write-with-response result=1`, `1E 01 01` sowie den ausgeschalteten
   Television-Service prüfen.

Eine Kompilierung impliziert keine Hardwarevalidierung.
