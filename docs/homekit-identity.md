# Deterministische HomeKit-Identität

homeBert leitet den achtstelligen HomeKit-Einrichtungscode und die vierstellige
QR-Setup-ID deterministisch aus der werkseitig programmierten ESP32-Basis-MAC
ab. Die Werte benötigen keine NVS-Speicherung und bleiben deshalb über
Neustarts, Firmware-Updates und ein vollständiges Löschen des Flashs hinweg
stabil.

## Verfahren v1

Für den Einrichtungscode und die Setup-ID werden getrennte SHA-256-Eingaben
aus einer festen ASCII-Domain, den sechs unveränderten Basis-MAC-Bytes in
Espressif-Reihenfolge und einem Zähler gebildet. Der Code verwendet die ersten
geeigneten acht Dezimalstellen; unzulässige HomeKit-Codes werden durch einen
deterministischen weiteren Zähler verworfen. Die Setup-ID wird aus einer
getrennten Domain in vier von HomeSpan akzeptierte Zeichen umgesetzt.

Die Domains und die genaue Umsetzung sind im Firmware-Quellcode dokumentiert.
Eine Änderung dieses Verfahrens würde nach einem Neu-Provisionieren gedruckte
QR-Labels ungültig machen und ist deshalb eine Kompatibilitätsentscheidung.

## Synthetischer Testvektor

Die Host-Tests verwenden ausschließlich eine lokal verwaltete,
nicht-einsatzbezogene Test-Basis-MAC und prüfen daraus Code, Setup-ID und
QR-Payload. Reale Gerätekennungen, Pairing-Codes und QR-Payloads gehören nicht
in diese Dokumentation.

## Migration

Beim ersten Start einer Firmware mit diesem Verfahren richtet homeBert den
HomeSpan-SRP-Verifikator für den abgeleiteten Code ein. Ein
versionsgebundener Migrationsmarker verhindert eine unnötige Wiederholung.
Bei bestehenden Controllerpaarungen wird der Pairing-Code nicht stillschweigend
geändert; die Firmware bewahrt Zubehöridentität und Controllerdaten.

Die aktuelle Setup-Information steht beim Boot, im lokalen Einrichtungsdialog
und über den seriellen Befehl `@p` zur Verfügung. Sie wird nicht in normalen
Statusausgaben protokolliert.
