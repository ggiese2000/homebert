# homeBert

### Dein Nubert nuXinema. Jetzt auch in Apple Home. ❤️

**Ein kleiner Helfer, der den nuXinema preAV endlich ins Smart Home bringt. Ganz ohne Cloud und auf Wunsch auch ohne Homebridge, Home Assistant oder zusätzlichen Server.**

<p align="center">
  <img src="docs/assets/homebert.png" alt="homeBert verbindet Apple Home mit dem Nubert nuXinema" width="720">
</p>

## Warum homeBert?

Ich liebe meinen **Nubert nuXinema preAV**.

Er klingt großartig, bietet jede Menge Möglichkeiten und ist das Herzstück meines Heimkinos. Als Nubert-Fan bin ich mit dem Gerät wirklich glücklich.

Fast.

Denn ich bin auch begeisterter Smart-Home-Tüftler. Und genau da hört der Spaß leider auf.

Der nuXinema lässt sich nicht ohne Weiteres in meine Hausautomation integrieren. Keine offen nutzbare Schnittstelle, keine direkte HomeKit-Anbindung, keine vernünftige Möglichkeit, ihn in Automationen einzubeziehen.

Und wenn er gerade nicht auf einen HDMI-Port eingestellt ist, funktioniert nicht einmal das automatische Ein- und Ausschalten über HDMI-CEC zuverlässig.

Fazit: Ein fantastischer Receiver, der in meinem Smart Home ein ziemlich einsames Leben führt. 😢

**Das musste doch besser gehen.**

### One App to rule them all! 💍

Nubert hat für seine X-Produktfamilie eine ziemlich praktische Lösung: **X-Remote**.

Mit dieser kostenlosen App lassen sich zahlreiche Nubert-Geräte über Bluetooth Low Energy (BLE) steuern – darunter Lautsprecher der nuPro X- und XS-Serien, nuSub-Subwoofer, nuConnect-Verstärker, nuZeo-Lautsprecher und natürlich auch der **nuXinema preAV**.

Und X-Remote kann mehr, als nur Befehle senden: Die Kommunikation funktioniert bidirektional. Die App kann Einstellungen verändern und Informationen über den Zustand des verbundenen Geräts empfangen.

Wenn die Nubert-App mit meinem nuXinema sprechen kann, warum sollte das nicht auch Apple Home können?

Nach eingehender Analyse der Bluetooth-Kommunikation zwischen X-Remote und dem nuXinema entstand **homeBert** – eine kleine Bridge, die den Receiver mit Apple Home verbindet.

Keine Änderungen am Receiver. Keine Cloud. Keine zusätzliche Serverinstallation.

Nur ein winziger ESP32, der zwischen Bluetooth und Apple Home vermittelt.

Und plötzlich versteht sich mein Heimkino mit meinem Smart Home.

---

## Was bringt mir homeBert?

Mit homeBert erscheint dein nuXinema als Fernsehzubehör in Apple Home.

Du kannst ihn über die Home-App bedienen und in deine Automationen einbinden.

**Das funktioniert bereits:**

- **Ein- und ausschalten:** Receiver einschalten oder in Standby versetzen.
- **Eingänge wählen:** Zwischen den verfügbaren Quellen wechseln.
- **Lautstärke regeln:** Direkt über Apple Home.
- **Zustände synchronisieren:** Änderungen am Receiver oder über die Fernbedienung werden auch in Apple Home sichtbar.
- **Surround automatisch wählen:** Bei einer über Apple Home ausgelösten Eingangswahl wird für HDMI/eARC automatisch der Auto-Modus und für andere Eingänge Stereo gewählt.

Besonders wichtig war mir dabei, dass homeBert nicht einfach Befehle ins Blaue schickt.

**homeBert kommuniziert bidirektional mit dem nuXinema:** Steuerbefehle werden über Bluetooth übertragen, während Statusmeldungen des Receivers an Apple Home zurückgemeldet werden. Dadurch bleiben beide Systeme auch dann synchron, wenn der nuXinema über seine Fernbedienung bedient wird.

### So sieht das in Apple Home aus

<p align="center">
  <img src="docs/assets/homekit-uebersicht.png" alt="homeBert in Apple Home" width="280">
  <img src="docs/assets/homekit-steuerung.png" alt="Steuerung des nuXinema in Apple Home" width="280">
</p>

*Hinweis: Die Surround-Umschaltung erfolgt automatisch im Hintergrund. Einen eigenen Surround-Schalter gibt es in Apple Home derzeit nicht.*

---

## Was du benötigst

homeBert läuft auf einem **M5Stack ATOM Lite**.

Er verbindet sich über Bluetooth Low Energy (BLE) mit dem nuXinema und über dein WLAN mit Apple Home.

Die gesamte Kommunikation bleibt lokal.

Du benötigst:

- Einen **M5Stack ATOM Lite** (zzgl. USB-C-Stromversorgung)
- Ein **2,4-GHz-WLAN**
- Ein **iPhone oder iPad mit Apple Home**
- Deinen **Nubert nuXinema preAV**

**Weitere Nubert-Geräte:** Bislang wurde homeBert ausschließlich mit dem nuXinema preAV getestet. Da X-Remote auch andere Nubert-Geräte über Bluetooth steuert, ist eine Erweiterung von homeBert durchaus denkbar. Dafür müssen allerdings zunächst die jeweiligen Steuerbefehle und gegebenenfalls Unterschiede im Bluetooth-Protokoll untersucht werden.

**Du möchtest homeBert mit einem anderen Nubert-Gerät verwenden?** Melde dich gerne – ich helfe bei der Untersuchung und möglichen Anpassung.

---

## Vor der Installation

Bevor du homeBert einrichtest, solltest du einige Punkte kennen.

**Bluetooth ist nicht unbegrenzt gesellig.**  
Die offizielle Nubert-App und homeBert können nicht gleichzeitig dieselbe Steuerverbindung nutzen. Beende die Nubert-App daher vollständig, bevor du homeBert mit dem Receiver verbindest.

**Die Kommunikation bleibt lokal.**  
homeBert benötigt keine Cloud und keinen zusätzlichen Server. Die Verbindung zum Receiver erfolgt über Bluetooth, die Verbindung zu Apple Home über dein lokales WLAN.

**homeBert ist kein offizielles Nubert- oder Apple-Produkt.**  
Es handelt sich um ein unabhängiges Open-Source-Projekt ohne offiziellen Support durch Nubert oder Apple.

---

## Firmware installieren

**homeBert 0.4.0 funktioniert. Die Firmware existiert und ist getestet.**

Hier kommt leider der Teil, bei dem selbst die Vogonen anerkennend nicken würden.

Fertige Firmware-Dateien stehen derzeit noch nicht zum Download bereit. Vor ihrer Veröffentlichung müssen die Lizenz- und Weitergabebedingungen einiger verwendeter Softwarekomponenten geprüft werden.

Arduino-ESP32, ESP-IDF und ihre Abhängigkeiten bringen unterschiedliche Open-Source-Lizenzen mit. Diese Bedingungen müssen auch bei der Weitergabe fertig kompilierter Firmware korrekt berücksichtigt werden.

Bis die zuständige Abteilung für intergalaktische Firmware-Freigaben ihre Stempel gesetzt hat, gibt es homeBert deshalb zunächst als Quellcode.

Da ich fertige Firmware-Dateien aus Lizenzgründen derzeit noch nicht anbieten
kann, kannst du homeBert selbst aus dem Quellcode erstellen.

Dafür benötigst du einen Computer mit Arduino CLI und den passenden
ESP32-Bibliotheken. Die Entwicklung und die bisherigen erfolgreichen Builds
erfolgten mit Arduino CLI 1.3.1, Arduino-ESP32 3.3.8, HomeSpan 2.1.8 und
NimBLE-Arduino 2.5.1.

Lade den Quellcode herunter und folge der [Build-Anleitung](docs/build-from-source.md).
Das vorhandene Build-Skript übernimmt die Kompilierung und Erstellung der
Firmware-Dateien. Anschließend überträgst du die Firmware per USB auf deinen
M5Stack ATOM Lite.

**Was ist getestet?** homeBert 0.4.0 läuft auf meinem ATOM Lite. Die Firmware
lässt sich mit der genannten Entwicklungsumgebung erfolgreich kompilieren. Der
vollständige Installationsweg auf einem frisch eingerichteten Computer wurde
noch nicht separat getestet.

Wenn du beim Bauen oder Installieren Schwierigkeiten hast, melde dich gerne.

Die Hintergründe findest du unter [Warum gibt es noch keine Firmware-Dateien?](build/release/WHY_NO_BINARIES_YET.md).

Der Quellcode wird mit homeBert 0.4.0 öffentlich bereitgestellt. Fertige
Firmware-Binärdateien bleiben bis zum Abschluss der Lizenzprüfung ausdrücklich
ausgenommen.

---

## Einrichtung: in wenigen Schritten zum Smart Home

Falls noch nicht geschehen, installiere zunächst die Firmware wie im vorherigen Abschnitt beschrieben.

### 1. homeBert starten

Verbinde den ATOM Lite mit der Stromversorgung.

Nach dem Start versucht homeBert, eine Verbindung zum Receiver und zum konfigurierten WLAN herzustellen. Wenn noch keine WLAN-Konfiguration vorhanden ist, startet automatisch der Einrichtungsmodus.

### 2. Mit dem Setup-WLAN verbinden

homeBert öffnet ein eigenes WLAN namens **homeBert Setup**.

Verbinde dich mit diesem Netzwerk und öffne anschließend die Einrichtungsseite unter:

```text
192.168.4.1
```

### 3. Receiver auswählen

homeBert sucht nach erreichbaren Nubert-Geräten.

Wähle deinen nuXinema aus der Liste aus.

Beende vorher die offizielle Nubert-App vollständig. Der Receiver erlaubt praktisch nur eine aktive Bluetooth-Steuerverbindung.

### 4. Heim-WLAN einrichten

Wähle dein 2,4-GHz-WLAN aus und gib das Passwort ein.

Anschließend beendet homeBert den Setup-Modus und verbindet sich mit deinem Heimnetzwerk.

### 5. Zu Apple Home hinzufügen

Jetzt zeigt homeBert deinen gerätespezifischen HomeKit-Code beziehungsweise QR-Code an.

Damit kannst du den Receiver in Apple Home hinzufügen.

**Fertig!**

Die ausführliche Anleitung, einschließlich Fehlerbehebung und Wiederherstellung, findest du unter [Einrichtung und Wiederherstellung](docs/setup-flow.md).

---

## Ein Knopf, eine LED – und ein bisschen Magie

Der ATOM Lite hat eine mehrfarbige LED und einen großen Taster auf der Vorderseite.

Damit lässt sich homeBert auch ohne Bildschirm bedienen.

<p align="center">
  <img src="docs/assets/homeBert-Schnellu%CC%88bersicht.png" alt="Taster und LED des M5Stack ATOM Lite" width="720">
</p>

**Die wichtigsten Funktionen:**

| Aktion | Was passiert? |
|---|---|
| Start | Kurze weiße Anzeige, anschließend Verbindungsaufbau |
| Verbindung hergestellt | LED leuchtet etwa fünf Sekunden grün und geht dann aus |
| Fronttaste kurz drücken | Aktuellen Status für fünf Sekunden anzeigen |
| Fronttaste doppelt drücken | Bluetooth-Verbindung neu aufbauen |
| Fronttaste zehn Sekunden halten | Werkseinstellungen wiederherstellen |
| Kleine seitliche Taste | ESP32 neu starten, ohne die Konfiguration zu löschen |

Die LED zeigt außerdem Verbindungsaufbau, Einrichtung und Fehlerzustände an.

Wer andere Farben oder ein anderes LED-Verhalten möchte, kann das über den Firmware-Quellcode anpassen.

Alle Einzelheiten findest du unter [LED und Taster](docs/LED_AND_BUTTON.md).

---

## Bekannte Einschränkungen und wichtige Hinweise

**Bluetooth-Verbindung mit der Nubert-App**

Die offizielle Nubert-App und homeBert können nicht gleichzeitig dieselbe Bluetooth-Steuerverbindung verwenden. Wenn du die App nutzen möchtest, musst du die Verbindung von homeBert gegebenenfalls unterbrechen.

**Apple Home kann Zustände zwischenspeichern**

Darstellung und Bezeichnungen der Eingänge werden teilweise von Apple verwaltet und zwischengespeichert. Änderungen erscheinen deshalb nicht immer sofort in der Home-App.

**Nicht alle Sonderfälle sind vollständig getestet**

Insbesondere die automatische Wiederherstellung der Bluetooth-Verbindung nach einem vollständigen Stromverlust des Receivers benötigt weitere Praxistests.

**Andere Nubert-Geräte sind nicht automatisch kompatibel**

Bislang wurde homeBert ausschließlich mit dem nuXinema preAV getestet. Die Unterstützung anderer Geräte kann nicht vorausgesetzt werden.

**Kein offizielles Produkt**

homeBert ist ein unabhängiges Community-Projekt und wird weder von Nubert noch von Apple unterstützt oder herausgegeben.

---

## Was kommt als Nächstes?

homeBert 0.4.0 ist der Anfang.

Ich möchte das Projekt schrittweise weiterentwickeln.

Auf meiner Wunschliste stehen unter anderem:

- Eine **Advanced-Edition** mit zusätzlichen HomeKit-Bedienelementen.
- Eine mögliche **Home-Assistant-Integration**.
- Weitere Verbesserungen bei Diagnose, Bedienung und Zuverlässigkeit.
- Fertige Firmware-Dateien, sobald die Lizenzprüfung abgeschlossen ist.

Diese Funktionen sind noch nicht Bestandteil der aktuellen Version.

---

## Für alle, die es genauer wissen möchten

Du möchtest wissen, wie homeBert intern funktioniert, wie die Bluetooth-Kommunikation aufgebaut ist oder welche Sicherheitsregeln implementiert sind?

Hier findest du die technischen Details:

| Thema | Dokumentation |
|---|---|
| Einrichtung und Wiederherstellung | [Setup-Anleitung](docs/setup-flow.md) |
| LED und Taster | [Bedienung](docs/LED_AND_BUTTON.md) |
| Bluetooth-Diagnose | [Serielle Konsole](docs/SERIAL_CONSOLE.md) |
| HomeKit und Bluetooth | [HomeKit-/BLE-Architektur](docs/phase-2-homekit.md) |
| Systemarchitektur | [Architektur](docs/architecture.md) |
| Firmware-Build und Versionierung | [Versionierung](docs/versioning.md) |
| Aktueller Entwicklungsstand | [Release Notes 0.4.0](docs/release-notes-0.4.0.md) |
| Veröffentlichung und Datenschutz | [Publication Review](docs/publication-review.md) |
| Lizenz- und Weitergabebedingungen | [Lizenzdokumentation](docs/licensing-and-redistribution.md) |
| Drittanbieterhinweise | [Third-Party Notices](THIRD_PARTY_NOTICES.md) |

---

## Open Source und Lizenz

Der von mir entwickelte homeBert-Quellcode steht unter der [MIT-Lizenz](LICENSE).

Verwendete Bibliotheken und Frameworks unterliegen ihren jeweiligen Lizenzen. Die Bedingungen für die öffentliche Weitergabe fertig kompilierter Firmware werden noch geprüft.

---

### Viel Spaß mit homeBert!

Vielleicht geht es dir wie mir: Du hast einen großartigen Receiver und möchtest ihn einfach ein bisschen besser in dein Zuhause integrieren.

Genau dafür habe ich homeBert gebaut.

**Für mehr Smart Home. Und noch mehr Freude am nuXinema.** ❤️
