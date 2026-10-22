# Serielle Konsole

Die serielle Konsole ist ein Wartungs- und Diagnosewerkzeug. Die installierte HomeSpan-Version 2.1.8 ist der **einzige** Leser der seriellen Eingabe. homeBert ergänzt die native HomeSpan-Hilfe ausschließlich über `SpanUserCommand`.

## Verbindung

Das ATOM Lite per USB anschließen und eine serielle Verbindung mit 115200 Baud, 8 Datenbits, keiner Parität und einem Stoppbit öffnen. Beispiel mit Python und pyserial:

```sh
python -m serial.tools.miniterm /dev/cu.usbserial-XXXXXXXX 115200 --eol LF
```

Unter Linux lautet das Gerät üblicherweise `/dev/ttyUSB0` oder `/dev/ttyACM0`. `LF` beendet einen Befehl; die HomeSpan-Konsole akzeptiert auch `CR` und `CRLF`. Miniterm wird mit `Ctrl+]` beendet. Niemals Logausgaben oder mehrzeiligen Text in die interaktive Konsole einfügen: jede vollständige Zeile kann ein Befehl sein.

`?` mit anschließendem Zeilenende zeigt die vom tatsächlich installierten HomeSpan bereitgestellte Befehlsliste einschließlich der homeBert-Befehle.

## Native HomeSpan-Befehle

Die folgenden Befehle stammen unverändert aus HomeSpan 2.1.8. Groß- und Kleinschreibung sind relevant. Einige verwalten HomeSpan-eigene NVS-Daten und sind **keine** reguläre homeBert-WLAN-Verwaltung, weil homeBert WLAN und BLE-Auswahl im NVS-Namensraum `homebert` selbst verwaltet.

| Syntax | Zweck | Wirkung/Risiko |
|---|---|---|
| `c`, `s`, `i`, `d`, `m`, `p` | Konfiguration, Verbindungsstatus, HAP-Datenbank, Heap bzw. Partitionen anzeigen | Nur lesend. `d` gibt die HAP-Attributdatenbank aus. |
| `Z` | WLAN-Netze scannen | Nur lesend, kann den WLAN-Stack kurz belasten. |
| `D` | WLAN trennen/erneut verbinden | Zustandsändernd am globalen WLAN-Stack; kein empfohlener homeBert-Recoverypfad. |
| `L <level>` | HomeSpan-Loglevel setzen (`-1` bis `2`) | RAM-Laufzeitoption von HomeSpan, unabhängig von `@L`. `L 2` kann umfangreiche HAP-Diagnostik und sensible Protokolldetails ausgeben; nicht dauerhaft oder in öffentlichen Logs verwenden. |
| `W` | HomeSpan-WLAN-Zugangsdaten interaktiv setzen | Schreibt HomeSpan-eigene WLAN-Daten und startet neu. Verwaltet nicht die homeBert-Zugangsdaten; nicht zur normalen Einrichtung verwenden. |
| `X` | HomeSpan-WLAN-Zugangsdaten löschen | Löscht nur HomeSpan-eigene WLAN-Daten und startet neu. Keine reguläre homeBert-WLAN-Wiederherstellung. |
| `A` | HomeSpan-Setup-Access-Point starten | Kann HomeSpan-eigene Konfiguration ändern und startet anschließend neu. Nicht für den homeBert-Setup-AP verwenden. |
| `S <code>` | HomeKit-Einrichtungscode ändern | Zustandsändernd: kann den deterministischen homeBert-Code und gedruckte QR-Labels inkonsistent machen. Nicht verwenden. |
| `Q <id>` | HomeKit-QR-Setup-ID (vier alphanumerische Zeichen) ändern | Zustandsändernd: kann die deterministische homeBert-Setup-ID und QR-Labels inkonsistent machen. Nicht verwenden. |
| `O` | OTA-Passwort ändern | Zustandsändernd; für normale Nutzung nicht erforderlich. |
| `U` | HomeKit-Controllerpaarungen löschen | Destruktiv für Apple-Home-Paarungen, erhält aber homeBert-WLAN, BLE-Auswahl und Zubehöridentität. Nur für bewusstes Pairing-Recovery nutzen. |
| `V` | gespeicherte HomeSpan-Charakteristikwerte löschen | Destruktiv für HomeSpan-Werte. |
| `H` | HomeKit-Geräte-ID und Controllerdaten löschen, Neustart | Destruktiv; bestehendes Apple-Home-Pairing wird ungültig. |
| `P`, `C` | HomeKit-Pairingdaten ausgeben bzw. klonen | Sensible Pairingdaten. Nicht in öffentliche Logs übernehmen; `C` ist zustandsändernd. |
| `R` | Neustart | Erhält Konfiguration. |
| `F` | HomeSpan-Werksreset, Neustart | Destruktiv für HomeSpan-Daten. |
| `E` | gesamtes NVS löschen, Neustart | Sehr destruktiv: entfernt alle gespeicherten Daten, einschließlich homeBert-Konfiguration. |
| `?` | HomeSpan-Hilfe | Nur lesend. |

Insbesondere `W`, `X`, `D` und `A` sind nicht als homeBert-WLAN-Einrichtung oder -Recovery dokumentiert. Für eine fehlende oder erstmals fehlgeschlagene homeBert-WLAN-Konfiguration ist der Access Point `homeBert Setup` unter `http://192.168.4.1` der unterstützte Weg. Siehe [Einrichtung und Wiederherstellung](setup-flow.md).

## homeBert-Befehle

| Syntax | Zweck | Wirkung |
|---|---|---|
| `@n` | homeBert-/nuXinema-Status ausgeben | Lesend; fordert bei bestehender BLE-Verbindung zusätzlich eine lesende Statusabfrage an. |
| `@p` | deterministischen HomeKit-Setup-Code, Setup-ID und `X-HM://`-Payload ausgeben | Nur lesend; verändert weder SRP-Daten noch Controllerpaarungen. |
| `@L` | homeBert-Diagnosestatus anzeigen | Nur lesend. |
| `@L0` | zusätzliche homeBert-Diagnoseausgaben ausschalten | Setzt den flüchtigen homeBert-Level auf 0. |
| `@L1` | informative BLE-Diagnostik einschalten | Zeigt zeitgestempelte Verbindungsversuche, Verbindungen, Trennungen und Fehlergründe (dezimal und hexadezimal). |
| `@L2` | detaillierte BLE-Diagnostik einschalten | Zusätzlich konfigurierte und tatsächlich verwendete BLE-Adresse/-Typ, Versuchszähler sowie Scan-Lebenszyklus. |
| `@b` | BLE-Identitätsscan starten | Diagnostisch und nicht destruktiv: verbindet sich nicht, schreibt kein NVS und ändert weder Receiverauswahl noch HomeKit oder WLAN. |

`@L` ohne Ziffer zeigt unter anderem Uptime, WLAN-Status/IP/RSSI, HomeKit-Startstatus, BLE-Zustand, gespeicherte Peer-Identität, letztes Verbindungsziel, letztes BLE-Ereignis samt Alter, letzten Fehlergrund sowie den Status eines Diagnosescans. Nur `@L`, `@L0`, `@L1` und `@L2` sind gültig; `@L 1` ist absichtlich keine gültige homeBert-Syntax. Der Level ist nur im RAM und fällt nach einem Neustart auf `0` zurück. Er ist vollständig unabhängig von HomeSpans `L <level>`.

## BLE-Verbindungsfehler untersuchen

Wenn die nuXinema nach einem Stromverlust nicht wieder verbunden wird, zuerst `@L2` und anschließend `@b` eingeben. `@b` wird bei einer laufenden Verbindung abgewiesen. Läuft gerade ein asynchroner Verbindungsversuch, wird exakt ein Scan vorgemerkt und erst nach dessen Abschluss gestartet. Während er vorgemerkt ist oder läuft, pausiert ausschließlich der nächste automatische Reconnect; nach Erfolg, Fehlschlag oder Timeout läuft der bestehende Reconnect-Mechanismus unverändert weiter.

Der Scan dauert ungefähr fünf Sekunden, leert alte Ergebnisse und zeigt maximal acht connectbare Ergebnisse mit Name, Adresse, Adresstyp, RSSI und `exact-match=yes/no`. Ein Treffer ist nur dann exakt, wenn **Adresse und Adresstyp** beide mit der gespeicherten nuXinema-Auswahl übereinstimmen:

- `exact-match=yes`: Die aktuell beworbene Identität stimmt mit der gespeicherten Identität überein; die Ursache liegt nicht in einer sichtbaren Abweichung dieser beiden Werte.
- Kein exakter Treffer, aber gleicher Name: Das ist nur ein Hinweis. homeBert wählt niemals automatisch ein anderes Gerät und speichert nichts.
- Kein passendes Gerät: Empfang, Werbung des Receivers oder eine geänderte Identität sind mögliche Untersuchungsansätze; der Scan allein beweist keine Ursache.

Mit `@L0` kehrt die zusätzliche homeBert-Diagnostik zu Normalbetrieb zurück. Die bereits vorhandenen wichtigen Status- und Fehlermeldungen bleiben auf allen Levels sichtbar.

Der wiederkehrende HomeSpan-Fehler zu einer fehlerhaften HTTP-Anfrage ist davon getrennt. HomeSpan 2.1.8 stellt die internen Parserwerte hierfür nicht über eine öffentliche API bereit. Falls erforderlich, kann sein natives `L <level>` manuell gesetzt werden; homeBert schaltet es nie automatisch ein.
