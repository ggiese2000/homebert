# Phase 2 native HomeKit integration

## Accessory model

One HomeKit accessory contains a native Television service with nine linked
InputSource services and a linked TelevisionSpeaker for the iOS Remote's
relative volume buttons. `Active` represents verified receiver power and
`ActiveIdentifier` represents the verified input. The stable HomeKit input
identifiers retain their BLE-value mapping (`identifier - 1`), while
`DisplayOrder` presents the physical remote order: eARC, HDMI 1, HDMI 2,
HDMI 3, Coax, Bluetooth, AUX, Optical, Port. This order follows Nubert's
documented remote control grouping: HDMI, Coax, Bluetooth, AUX, Opto, Port.

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

The Nubert remote order is documented in the [nuXinema preAV operating
instructions](https://www.nubert.de/down/ab/e2/5a/1690272485/nuxinema-preav-bedienungsanleitung.pdf).
The identifiers deliberately remain unchanged; each maps to its original BLE
value through `identifier - 1`. AUX is hardware-verified. The remaining input
writes retain their observed protocol mapping and require individual hardware
validation before being described as hardware-verified.

The TelevisionSpeaker service has no readable absolute-volume characteristic.
The same accessory therefore adds a small compatible control:

- `Volume` is a Fan `RotationSpeed` control, mapped directly to the receiver's
  raw `0x00`–`0x64` volume value (dB = raw - 100). Fan `Active` is ignored:
  it neither changes receiver power nor sends a volume command.

The former LightBulb service is replaced by a Fan service. Existing HomeKit
pairing is preserved by firmware; after restart Apple Home should refresh the
accessory database. If it retains the former light tile, remove and re-add
homeBert in Apple Home. This is an Apple Home service-cache migration, not a
firmware factory reset; Wi-Fi and receiver configuration remain untouched.

There is no separate surround control. An explicit HomeKit input change queues
the matching surround write only after the receiver confirms that input: Auto
(`05`) for eARC and HDMI 1–3, Stereo (`00`) for all other inputs. Remote and
physical input/surround changes never trigger automatic surround writes.

## State and command handling

The central state model records BLE connection state plus valid power, volume,
input, and surround values. It is updated from standalone three-byte records
and `86` bundled TLV responses. On every GATT connection, the firmware queues
the main and sound-settings status requests after notification subscription.

BLE notifications never call HomeSpan directly. They mark state dirty; the main
loop publishes only confirmed, valid values to HomeKit. Disconnect clears all
validity flags and sets power to UNKNOWN. Cached HomeKit values are not
republished as verified state while disconnected.

HomeKit callbacks only queue one BLE write; they never call `setVal()` while
HomeSpan is processing that characteristic. The main loop sends the write and
queues both status queries 500 ms later. A successful GATT write is not treated
as confirmation. Für Lautstärke und Power gilt zusätzlich eine begrenzte,
flüchtige Pending-Reconciliation: Wird ein Befehl in die Queue aufgenommen,
akzeptiert HomeSpan die Benutzeranfrage zunächst. Veraltete Rückmeldungen
überschreiben den angeforderten Wert nicht; erst eine passende bestätigte
Empfänger-Rückmeldung, Fehler, Disconnect oder Timeout beendet den Pending-
Zustand. Dadurch wird weder ein GATT-Write noch das Absenden selbst mit einer
Gerätebestätigung verwechselt. Es gibt kein periodisches BLE-Polling.

If Apple Home sends an input request during an accepted HomeKit Power-ON
transition, homeBert holds that request for at most ten seconds. It sends the
input only after the receiver confirms ON, then retains the existing confirmed-
input-to-surround sequence. The deferred request is cancelled on timeout,
disconnect, UNKNOWN state, or a HomeKit standby request; it is never replayed
after a later unrelated wake-up.

## BLE command evidence

| Function | Status | Write | Phase 2 use |
|---|---|---|---|
| Power ON | `1E 01 00` | confirmed `1F 01 00` | Enabled only from verified STANDBY |
| Power OFF / STANDBY | `1E 01 01` | hardware-verified `1F 01 01` | Enabled only from verified ON |
| Volume | `0A 01 value` | hardware-verified `0B 01 value` | Enabled from verified ON |
| Input | `0E 01 value` | `0F 01 value` | Enabled; AUX (`04`) was independently tested |
| Surround | `93 01 value` | `94 01 value` | Automatic Stereo (`00`) and Auto (`05`) after HomeKit input confirmation |

The main status query and the sound-settings query are confirmed captured
read-only requests. `1F 01 01` was hardware-verified by write response,
notification, and a subsequent status query. Volume writes and the automatic
Stereo (`94 01 00`) and Auto (`94 01 05`) surround writes were subsequently
verified on the target receiver. AUX input selection was independently tested;
the remaining input values are observed status mappings using the same command
format and remain candidates for per-input hardware validation.

## Safety policy

Input, volume, and surround writes require verified ON state. Power ON is the
only exception and requires verified STANDBY; it is never inferred from BLE
connection or write success. HomeKit Power OFF sends the verified `1F 01 01`
only while ON and awaits a confirmed `1E 01 01` notification or refresh. A
queued command prevents another command from being sent until it has been
attempted and a targeted refresh has been queued.

## Hardware validation

1. Flash at 115200 baud using the existing Phase 1C procedure, then provision
   Wi-Fi and select the receiver.
2. Confirm HomeSpan starts and pair `homeBert` in Apple Home using the
   deterministic eight-digit setup code or QR payload from serial output. Both
   values derive from the ESP32 base MAC; see `docs/homekit-identity.md`.
3. Confirm GATT subscription, both status-query write results, and power,
   volume, input, and surround logs. Confirm their matching HomeKit controls
   only after the relevant BLE notification arrives.
4. Change power, volume, input, and surround from the Nubert remote and confirm
   HomeKit updates without issuing a HomeKit command.
5. From HomeKit, request Power ON and an input during the transition. Confirm
   `HomeKit input deferred`, then an input write only after `1E 01 00`. Confirm
   the mapped surround write follows the confirmed input.
6. Verify the displayed order eARC, HDMI 1, HDMI 2, HDMI 3, Coax, Bluetooth,
   AUX, Optical, Port. Verify AUX on hardware; the other input writes remain
   protocol-derived candidates for individual hardware validation.
7. Confirm Fan speed controls receiver volume and receiver volume changes
   update Fan speed. Change Fan Active and confirm no BLE write or receiver
   power change. Confirm Apple's "all lights off" action no longer includes
   Volume.
8. While STANDBY or UNKNOWN, attempt volume, input, and surround changes and
   confirm `Nubert device command blocked` is logged with no GATT write.

9. From HomeKit, switch the Television off while ON. Confirm `Power STANDBY
   write-with-response result=1`, `1E 01 01`, and the Television becomes off.

No hardware validation is implied by compilation.
