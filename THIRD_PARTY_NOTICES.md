# Third-party notices

homeBert source code is licensed under the project [MIT License](LICENSE).
The firmware image is built from third-party components under their own
licenses. Those licenses do not grant rights to the names or trademarks of
their authors.

| Component | Pinned version | License | Notice / source |
|---|---:|---|---|
| HomeSpan | 2.1.8 | MIT | Copyright (c) 2020–2025 Gregg E. Berman; full license is included in the HomeSpan distribution. |
| NimBLE-Arduino | 2.5.1 | Apache-2.0 | Copyright and NOTICE material are retained in its upstream distribution; see [Apache License 2.0](https://www.apache.org/licenses/LICENSE-2.0). |
| Arduino-ESP32 | 3.3.8 | LGPL-2.1-or-later and component-specific licenses | Built from the Espressif Arduino core and ESP-IDF components; see the [Arduino-ESP32 repository](https://github.com/espressif/arduino-esp32) and its included license/notice material. |
| Espressif tools and ESP-IDF components | supplied by Arduino-ESP32 3.3.8 | component-specific | Distributed only as build dependencies; release consumers should consult the notices included with the corresponding Arduino-ESP32 package. |

## Binary redistribution

The generated firmware is a combined binary. Before publishing a binary as a
GitHub Release asset, retain the applicable license and NOTICE files supplied
with the exact Arduino-ESP32, HomeSpan, and NimBLE-Arduino distributions used
to build it. This repository does not vendor those dependencies; the pinned
versions are installed by `tools/build.sh`.

The HomeSpan MIT license and NimBLE-Arduino Apache-2.0 license texts are
available in their installed/upstream distributions. The Arduino-ESP32 package
contains multiple Espressif and third-party components with separate notices;
their complete binary-redistribution obligations require review for the exact
published artifact.
