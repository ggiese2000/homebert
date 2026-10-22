# Versionierung und lokale Releases

Die einzige editierbare Firmware-Version steht in der Datei
[`VERSION`](../VERSION) im Repository-Wurzelverzeichnis und folgt Semantic
Versioning. Der aktuelle Stand `0.4.0` ist ein **unveröffentlichter
Release-Kandidat**. Er darf erst nach Hardware-, Privacy- und Lizenzfreigabe
getaggt oder veröffentlicht werden; der Status steht zusätzlich in
[Release-Readiness](release-readiness.md).

`./tools/build.sh` prüft das Versionsformat, kompiliert die Version zusammen
mit dem aktuellen Git-Commit in die Firmware und erzeugt unter `build/` eine
ignorierte Build-Metadatendatei. Die Firmware protokolliert Version und Commit
beim Start; HomeKit zeigt die Version als `FirmwareRevision` und HomeSpan
veröffentlicht sie als Sketch-Version per mDNS.

`./tools/release.sh` setzt einen sauberen Git-Arbeitsbaum voraus, baut erneut
und erzeugt die zwei bekannten Images sowie `SHA256SUMS` und `BUILD_INFO` in
`build/release/`. `BUILD_INFO` enthält Version, Commit, Build-Konfiguration,
Imagegrößen und SHA-256-Werte. Diese Dateien sind lokal und werden nicht
eingecheckt.

Vor einem späteren Release: `VERSION` ändern, `CHANGELOG.md` und die
Release-Readiness aktualisieren, bauen, auf Hardware validieren,
`build/release/BUILD_INFO` und `SHA256SUMS` prüfen, den Arbeitsbaum prüfen und
erst dann einen Git-Tag sowie eine Veröffentlichung bewusst freigeben. Die
Artefakte bleiben bis dahin ignoriert und werden nicht eingecheckt.
