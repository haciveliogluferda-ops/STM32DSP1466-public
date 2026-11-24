# Linkplay MCU Command Map

This document collects the Linkplay/MCU commands used by the STM32DSP1466 firmware, reverse-engineering notes for the AX31, and additional commands from the A98 MCU instruction PDF.

- `Firmware` refers to behavior implemented in `src/main.cpp`.
- `AX31 RE` refers to behavior captured on the Up2Stream/A31 and recorded in `AX31UARTAPI.xlsx`.
- `A98 PDF` refers to the newer protocol described in `Instructiondocument.pdf`. It is a reference for command meaning and sequence. Check compatibility on the A31.

Use the UART settings in this firmware for the A31 module in this project. The generic A98 settings apply to a different module.

## Commands currently used by the firmware

| Direction | Command | Firmware behavior | Notes |
| --- | --- | --- | --- |
| MCU -> LP | `MCU+VOL+%03u` | Reports the current volume to LP. | Used during initialization and in response to `AXX+VOL+GET`. |
| LP -> MCU | `AXX+VOL+GET` | Replies with `MCU+VOL+%03u`. | Matches the A98 PDF. |
| LP -> MCU | `AXX+VOL+nnn` | The STM32 applies the volume and saves it to EEPROM. | Clamped to `0..100`. |
| MCU -> LP | `MCU+PLM-xxx` | Reports the current source or mode to LP. | The firmware uses the `-` form. During standby or boot, it reports `000` even if the last saved source was BT, XLR or Optical. For the Network source it may report a known last service mode such as `031/010/001`. |
| LP -> MCU | `AXX+PLM+GET` | Experimentally replies with `MCU+PLM+xxx`. | The Spotify standby wake sequence tests the `+` form in place of the `-` form. The boot message `MCU+VER...&MCU+PLM-xxx` stays the same. |
| LP -> MCU | `AXX+PLM+nnn` | Selects the source according to the LP mode. | `031 Spotify`, `001 AirPlay` and `010 Radio` identify services within Network, so the DSP source stays Network. Other modes are `040 Line In`, `041 Bluetooth`, `043 Optical`, `048 XLR` and `000 Network`. |
| MCU -> LP | `MCU+VER+0035&MCU+PLM-xxx` | Reports the MCU firmware version and mode together. | Sent after `AXX+MCU+VER` and `AXX+MCU+RDY`, immediately followed by `MCU+CAP+PLM00008214&`. |
| LP -> MCU | `AXX+MCU+VER` | Sends the version and mode response. | A normal boot query in the A98 PDF. |
| LP -> MCU | `AXX+MCU+RDY` | On the first ready notification, reports version, capabilities, volume and EQ. | Signals that Linkplay is ready. `AXX+MEA+RDY` signals metadata readiness. |
| MCU -> LP | `MCU+RTC+GET` | Requests network time from LP. | Used in response to `AXX+GET+RTC`. LP may first send `AXX+RTC+NIL`, then `AXX+SET+RTC...` once it has NTP time. |
| MCU -> LP | `MCU+MEA+GET` | Requests metadata. | Can be followed by `MCU+PINFGET`. |
| LP -> MCU | `AXX+MEA+DAT{...}&` | Decodes hex metadata for display. | Title, artist, album and vendor. |
| LP -> MCU | `AXX+MEA+RDY` | Treats media or player information as ready and triggers `MCU+PINFGET`. | Sending `MCU+MEA+GET` again caused a loop in Spotify standby wake tests. |
| MCU -> LP | `MCU+PINFGET` | Requests player or session information. | Expects `AXX+PLY+INF{...}&`. |
| LP -> MCU | `AXX+PLY+INF{...}&` | Processes player information, volume, mute, timing and metadata. | Relevant to sources such as Spotify. |
| LP -> MCU | `AXX+PLY+INF{..."loop":"0/1/3/4"...}` | Reports repeat or shuffle status. | The firmware shows icons for `0=repeat all/album`, `1=repeat one` and `3=shuffle/random`. It shows no loop icon for `4=normal/clear` or unknown value `2`. The mute icon appears first when present. |
| MCU -> LP | `MCU+PLY+PUS` | Toggles play/pause or pauses before standby. | Used for Network/Linkplay transport. On entering standby, it is sent only if the last known state was `play`. Sending it in the `pause/stop` state can restart Spotify. |
| MCU -> LP | `MCU+PLY+NXT` | Skips to the next track. | Used by IR and the terminal. Can also be sent on the standby screen when Network is the current source. |
| MCU -> LP | `MCU+PLY+PRV` | Skips to the previous track. | Used by IR and the terminal. Can also be sent on the standby screen when Network is the current source. |
| LP -> MCU | `AXX+PLY+001` | Playback started. | A primary trigger for waking from standby on the Network source. |
| LP -> MCU | `AXX+PLY+000` | Treated as pause. | |
| LP -> MCU | `AXX+SPY+001` | Indicates an active Spotify session. | Can also arrive when selecting the device. It does not trigger auto-wake. |
| LP -> MCU | `AXX+PLM+031` | Reports Spotify mode. | Selects Network and requests metadata. The Spotify icon or label is chosen after the vendor in `MEA+DAT` or the mode in `PLY+INF` is known. The Spotify standby wake sequence tests `MCU+PLM+031` as the reply to `PLM+GET`. |
| LP -> MCU | `AXX+PLY+INF{..."status":"play"...}` | Reports actual playback. | A primary auto-wake trigger. |
| MCU -> LP | `MCU+MUT+001` / `MCU+MUT+000` | Reports mute state to LP. | Sent when mute is toggled. |
| LP -> MCU | `AXX+MUT+001` / `AXX+MUT+000` | The STM32 applies the mute state. | |
| LP -> MCU | `AXX+WWW+001` / `AXX+WWW+000` | Tracks Wi-Fi readiness. | Updates `linkplayWifiReady`. |
| LP -> MCU | `AXX+SET+RTCYYYYMMDDHHMMSS&` | Synchronizes the STM32 clock from the incoming UTC RTC string. | The A98 PDF describes the RTC and app time flow. |
| LP -> MCU | `AXX+NXT+ALS<n>&` | May report a daylight-saving or local-time offset. | `ALS-1` was interpreted as a one-hour DST difference and `ALS-2` as two hours. It can follow an RTC update. The firmware does not apply it separately. |
| LP -> MCU | `AXX+BOT+DON` / `AXX+BOT+UP0` | LP boot notification. | The firmware responds once with `MCU+BOT+DON`. |
| LP -> MCU | `AXX+PAS+EQGet&` | Reports the bass and treble settings. | |
| LP -> MCU | `AXX+PAS+EQSet:bass:n&` | Converts the value to bass gain and writes it to the DSP. | |
| LP -> MCU | `AXX+PAS+EQSet:treble:n&` | Converts the value to treble gain and writes it to the DSP. | |
| MCU -> LP | `MCU+PAS+EQSet:bass:nn&MCU+PAS+EQSet:treble:nn&` | Reports the EQ state to LP or the app. | The AX31 V3 notes report no response, but the firmware still sends it. |

## Commands observed in the AX31 reverse-engineering notes

| Direction | Command | Captured response or behavior | Status |
| --- | --- | --- | --- |
| MCU -> LP | `MCU+BOT+DON` | `AXX+BOT+DON`, then `AXX+BOT+UP0`. | Confirmed in the boot handshake. The firmware replies once after `AXX+BOT+DON/UP0`. |
| MCU -> LP | `MCU+POW+SAV` | `AXX+POW+001`, followed by `AXX+RA0+000`. | Appears to enter power-save on the A31. It may leave the module powered. |
| MCU -> LP | `MCU+WWW+GET` | `AXX+WWW+001`. | Queries Wi-Fi status. |
| MCU -> LP | `MCU+GET+UID` | `AXX+UID+...&`. | Used in the boot sequence. |
| MCU -> LP | `MCU+DEV+GET` | `AXX+DEV+INF...&`. | Short device information. |
| MCU -> LP | `MCU+INF+GET` | `AXX+INF+INF{...}&`. | Detailed device information. |
| MCU -> LP | `MCU+PLP+GET` | `AXX+PLP+004`. | Playback repeat or queue mode. |
| MCU -> LP | `MCU+PLP+000` | Sent during the boot sequence. | May report repeat or playback mode. |
| MCU -> LP | `MCU+EQ+GTYP` | Sent during the boot sequence. | May relate to the app's EQ type. |
| MCU -> LP | `MCU+VOL+030` | Sets volume. | Setter. |
| MCU -> LP | `MCU+VOL+GET` | `AXX+VOL+value`. | Queries the module's volume. |
| MCU -> LP | `MCU+VOL-100` | `AXX+VOL+100`. | Recorded as the MCU reporting its own volume to the module. |
| MCU -> LP | `MCU+PLM+000` | `AXX+PLM+000`, Network. | The `+` form switches the module's source. |
| MCU -> LP | `MCU+PLM+005` | `AXX+PLM+040`, Line In. | AX31 mapping for Aux-In. |
| MCU -> LP | `MCU+PLM+006` | `AXX+PLM+041`, Bluetooth. | AX31 mode mapping. |
| MCU -> LP | `MCU+PLM+008` | `AXX+PLM+043`, Optical. | AX31 mode mapping. |
| MCU -> LP | `MCU+PLM+010` | `AXX+PLM+045`, Coaxial. | Inactive in this project. |
| MCU -> LP | `MCU+PLM+012` | `AXX+PLM+047`, Line In 2. | Inactive in this project. |
| MCU -> LP | `MCU+PLM+013` | `AXX+PLM+048`, XLR. | Correct mapping for this project's XLR source once enabled via CAP+PLM. |
| MCU -> LP | `MCU+USB+GET` | `AXX+USB+000` / `001`. | USB detection query. In plug/unplug tests, `001` meant a drive was detected and `000` meant it was absent or removed. Use `PLY+INF mode` to determine the active playback source. |
| LP -> MCU | `AXX+PLY+INF{..."mode":"11"...}` | USB flash playback. | Observed with a compatible USB drive. The DSP still uses its Linkplay input and the firmware shows the logical mode as `USB`. |
| LP -> MCU | `AXX+PLY+INF{..."mode":"51"...}` | USB DAC variant. | Some Up2Stream boards may use a separate MCU/USB path. This project has no USB DAC hardware. Mode `11` identifies USB flash playback. |
| MCU -> LP | `MCU+MUT+GET` | `AXX+MUT+000` / `001`. | Queries mute status. |
| MCU -> LP | `MCU+PLY-PLA` | `AXX+MEA+RDY`. | Play command or playback-start variant. |
| MCU -> LP | `MCU+PLY-STP` | No response. | Stops current playback. |
| MCU -> LP | `MCU+PLY+PUQ` | `AXX+KEY+NIL`. | Function unclear or unavailable. |
| MCU -> LP | `MCU+KEY+001` | `AXX+KEY+NIL`. | Likely unimplemented. |
| LP -> MCU | `AXX+GET+RTC` | Arrives during boot. | The original capture notes record this as unanswered and a candidate for RTC handling. The later debug notes below record a firmware response. |
| LP -> MCU | `AXX+CHN+GET` | Arrives during boot. | Queries channel information. The response behavior was still to be confirmed at this point. |
| LP -> MCU | `AXX+CHN+001/002/000` | Left, right or stereo channel event. | Can come from the app. |
| LP -> MCU | `AXX+PMT+000/001` | Voice prompt start/end or status. | Seen during boot and app commands. |
| LP -> MCU | `AXX+SIL+001` | Recorded as a possible request to switch to Bluetooth. | The firmware currently ignores it. |
| LP -> MCU | `AXX+SIL+000` | Recorded as a possible request to switch to Line In. | The firmware currently ignores it. |
| LP -> MCU | `AXX+SLV+YES/000/001/NOT` | Multiroom master/slave status. | `001` means another device joined as a slave and displays `M`. `YES` means this device joined as a slave and displays `Slave`. `000/NOT` clears the indication. |
| LP -> MCU | `AXX+BT+GPRL` | Seen during boot. | May concern a Bluetooth profile or pairing list. Unconfirmed. |
| LP -> MCU | `AXX+FACTORY` | Seen during a factory reset. | The firmware currently ignores it. |
| LP -> MCU | `AXX+DEV+RST` | Seen after a restart or factory reset. | The original AX31 notes record it as unhandled. |
| LP -> MCU | `AXX+STA+001/002` | Network state events. | Used for intermediate boot-screen messages. `WWW` still determines Wi-Fi readiness. |

## Debug observations from 2026-08-24

These observations came from a live AX31 module with a clone ST-Link, debug UART and `lplog` enabled.

### Boot and ready

| Sequence | Observation | Result |
| --- | --- | --- |
| Cold boot or power cycle | `AXX+BOT+DON` and `AXX+BOT+UP0` appeared, followed by `AXX+MCU+RDY`. | `BOT` belongs to the cold-boot or heartbeat sequence. |
| Manual `lp MCU+BOT+DON` | LP replied with `AXX+BOT+DON`. When the firmware automatically sent `MCU+BOT+DON`, LP sent `AXX+BOT+DON` again. | `AXX+BOT+DON` can be both a boot notification and an acknowledgment of `MCU+BOT+DON`. Use a flag to prevent a reply loop. |
| `AXX+MCU+RDY` | On first readiness, the firmware reports `MCU+VER+0035&MCU+PLM-000`, `MCU+CAP+PLM00008214&`, volume, neutral EQ and the current EQ settings. | State reporting after ready works. Repeated RDY events still indicate module readiness. Metadata readiness has its own event. |

### Power

| Command | Observed response or event | Result |
| --- | --- | --- |
| `MCU+POW+STA` | `AXX+POW+000` | `000` means active or normal. |
| `MCU+POW+SAV` | `AXX+POW+001`, then `AXX+RA0+000` | Power-save or sleep. `RA0+000` may indicate AP or interface shutdown. |
| `MCU+POW+STA` after SAV | `AXX+POW+001` | Confirmed `001` as power-save. |
| `MCU+POW+RES` | `AXX+STA+000`, `AXX+POW+000`, followed by a burst of `STA/WWW/PMT/MUT` events. | Resumes from power-save without a full boot. The module sent no `AXX+BOT...`. |
| `MCU+POW+OFF` | `AXX+POW+OFF` after about 5.16 s. | The hard-off handshake works. A 2 s timeout is too short. Allow 8–10 s. |
| `MCU+POW+STA` after OFF with power still connected | `AXX+POW+000` | After `POW+OFF`, if the MCU leaves the rail powered, the module can return to active or at least report that state. |

After `MCU+POW+RES`, the “connected to your WiFi network” prompt may play once or twice. The log may contain two bursts of `WWW`, `PMT` and `MUT` events. When resuming from standby, send `POW+RES` if `POW+STA` returns `001`. Sending RES when the module already reports `000` can replay the prompt.

### RTC and time

| Sequence | Observation | Result |
| --- | --- | --- |
| During boot | `AXX+GET+RTC` arrived and the firmware replied with `MCU+RTC+GET`. | The firmware can request network time when it has no valid RTC time to report. |
| Early cold boot | `AXX+RTC+NIL` followed `MCU+RTC+GET`. | LP can return NIL before obtaining network or NTP time. |
| After Wi-Fi/NTP | `AXX+SET+RTC20260824185015&`, `AXX+SET+WEK1&` and in some cases `AXX+NXT+ALS-1&`. | LP pushes time once available. `WEK` appears to be the day of the week, and `ALS` a daylight-saving or local offset. The firmware processes the RTC string. It applies no separate `WEK/ALS` adjustment. |
| During playback | `lp MCU+RTC+GET` remains to be tested. | Time queries during playback are unverified. |

### Channel

| Sequence | Observation | Result |
| --- | --- | --- |
| During boot | `AXX+CHN+GET` arrived. | LP queries channel configuration. |
| Firmware response | `MCU+CHN+000` was sent. | The exchange continued normally with `000` as the stereo response. |

### Spotify and network playback

| Sequence | Observation | Result |
| --- | --- | --- |
| Select Spotify | `AXX+PLM+031` and `AXX+MEA+RDY` arrived, and the firmware sent `MCU+MEA+GET`. | `031` identifies Spotify within Network. The DSP stays on Network and the metadata exchange starts. |
| Metadata | `AXX+MEA+DAT{... "vendor":"53706F74696679" ...}&`, then `MCU+PINFGET`. | The vendor is Spotify. Title, artist and album can arrive in hex. |
| Player info | `AXX+PLY+INF{..."mode":"31","status":"pause/play",...}` | Provides player state, duration, position, volume and mute. |
| Playback start | `AXX+PLY+001` arrived and the firmware woke with `AMP_ENABLE ON from lp-ply`. | LP playback auto-wake works. |
| Concatenated frames | `AXX+SPY+001AXX+MEA+DAT...` was observed. | The parser uses `AXX+` boundaries to handle adjacent frames. It currently ignores `SPY+001`. |

On AX31, `AXX+MEA+RDY` signals that media information is ready. In Spotify standby wake tests, the firmware had already sent `MCU+MEA+GET` after `PLM+031`. Sending `MCU+MEA+GET` again on `MEA+RDY` could create a loop. The firmware now requests `MCU+PINFGET` directly on that event.

Example decoded metadata

- Title: `Kara Bahtima Kustum`
- Artist: `Tuncay Kemertas`
- Album: `Kayla`
- Vendor: `Spotify`

## Additional and candidate commands from the A98 PDF

Use these commands as a protocol and test reference. Their behavior on the older AX31 module still needs to be checked where noted.

### Boot and OOBE

| Direction | Command | Meaning in the PDF | Project notes |
| --- | --- | --- | --- |
| LP -> MCU | `AXX+CAP+GET` | Requests product capability information during factory setup or OOBE. | The firmware responds. It may not appear during a normal AX31 boot. |
| MCU -> LP | `MCU+CAP+PRJproject_name&` | Required project or product name. | Can be added if OOBE is needed. |
| MCU -> LP | `MCU+PTV+nnn` | MCU protocol version. | The AX31 firmware uses `MCU+VER+0035`. |
| MCU -> LP | `MCU+CAP+001xxxxxxxx&` | Capability bits. | Bit 16 indicates an MCU RTC. The capability notes record an RTC without a backup battery. See the later RTC decision below for the board configuration. |
| MCU -> LP | `MCU+CAP+002xxxxxxxx&` | Second set of capability bits. | The actual CAP2 value is unclear. Confirm it independently from `capability` in `INF`. |
| MCU -> LP | `MCU+CAP+LAUlanguage&` | Default language. | May be optional for this project. |
| MCU -> LP | `MCU+CAP+STMxxxxxxxx&` | Enables or disables streaming services. | Could help simplify the app's service list. |
| MCU -> LP | `MCU+CAP+PLMxxxxxxxx&` | Supported local input modes. | Tested `00008016` for Wi-Fi/BT/USB/Aux/Optical and `00008214` for Wi-Fi/BT/USB/XLR/Optical. The firmware uses `00008214`. This project has no USB DAC hardware. Use `PLY+INF mode=11` for USB flash playback. |
| MCU -> LP | `MCU+SID+name&` | Default SSID at the end of the capability responses. | The firmware returns `DSP_Amp`. |
| LP -> MCU | `AXX+BOT+DON` | The module is booting. | Matches AX31 observations. |
| LP -> MCU | `AXX+BOT+UP0` | The module has started. It can also follow the `MCU+BOT+DON` heartbeat or boot response. | Matches AX31 observations. |
| LP -> MCU | `AXX+MCU+RDY` | The module is ready to communicate with the MCU. | Handled by the firmware. |

### Power, sleep and firmware updates

| Direction | Command | Meaning in the PDF | Project notes |
| --- | --- | --- | --- |
| MCU -> LP | `MCU+POW+OFF` | Asks the Wi-Fi module to prepare for power-off. | On AX31, `AXX+POW+OFF` arrived about 5.16 s later. |
| LP -> MCU | `AXX+POW+OFF` | Acknowledges readiness for power-off. | The LP rail can be cut after this response. Allow an 8–10 s timeout. |
| LP -> MCU | `AXX+BURNING` | An OTA or firmware update is underway. Keep power connected. | Handle in the hard-off state machine. |
| LP -> MCU | `AXX+BURN002` | Listed as a variant for update or lighting effects. | Reference only. |
| MCU -> LP | `MCU+POW+SAV` | Enters sleep or power-save. | On AX31, this produced `AXX+POW+001` and `AXX+RA0+000`. |
| MCU -> LP | `MCU+POW+RES` | Resumes from sleep. | On AX31, this produced `AXX+POW+000` and a burst of network or prompt events. |
| MCU -> LP | `MCU+POW+STA` | Queries power state. | On AX31, `000` means active and `001` means power-save. |
| MCU -> LP | `MCU+FCRYPOW` | Requests factory restore and power-off. | Use only in test mode. |
| LP -> MCU | `AXX+DEV+RST` | Module restart notification. | A restart can clear UI or source state. |

### RTC, time and alarm

| Direction | Command | Meaning in the PDF | Project notes |
| --- | --- | --- | --- |
| LP -> MCU | `AXX+GET+RTC` | Requests the MCU's RTC time. | The firmware asks for network time with `MCU+RTC+GET`. |
| LP -> MCU | `AXX+SET+RTCYYYYMMDDHHMMSS&` | The Wi-Fi module or app sets the MCU clock. | Handled by the firmware. |
| LP -> MCU | `AXX+APP+RTCYYYYMMDDHHMMSS&` | RTC update from the app. | Currently unhandled. Could be processed like `SET+RTC`. |
| MCU -> LP | `MCU+RTC+GET` | Requests time from the Wi-Fi module. | Used in response to `AXX+GET+RTC` when network time is needed. |
| MCU -> LP | `MCU+SND+RTCYYYYMMDDHHMMSS&` | Sets the Wi-Fi module's RTC. | AX31 accepted it, but querying the time again with `MCU+RTC+GET` returned a value about six hours ahead of the input. Keep it out of automatic flows. |
| MCU -> LP | `MCU+WEK+GET` | Queries alarm or weekday information. | Low priority for this project. |

The STM32 hardware RTC runs from LSE and has an external battery supply on VBAT. The firmware uses it for the live clock and uses the external 25xx EEPROM for snapshots and fallback. When LP is online, the `AXX+SET+RTC...` response to `MCU+RTC+GET` is treated as an NTP time source. The MCU-to-LP clock setting path (`MCU+SND+RTC...`) is excluded from normal operation due to the offset observed on AX31.

### Bluetooth and source switching

| Direction | Command | Meaning in the PDF | Project notes |
| --- | --- | --- | --- |
| LP -> MCU | `AXX+BT+CON1` | Bluetooth connected. | The firmware also monitors a separate BT UART. This LP event could be supported. |
| LP -> MCU | `AXX+BT+CON0` | Bluetooth disconnected. | Candidate for UI or source-state handling. |
| MCU -> LP | `MCU+BT+CONN` | Requests reconnection to the last Bluetooth device. | The board has a separate BT UART. Test whether LP needs this command. |
| MCU -> LP | `MCU+BT+PAIR` | Requests Bluetooth pairing mode. | The board has a separate BT UART. This may be useful for LP app synchronization. |
| LP -> MCU | `AXX+SIL+001` | The AX31 notes suggest a request to switch to BT. | Retest alongside the A98 Bluetooth commands. |
| LP -> MCU | `AXX+SIL+000` | The AX31 notes suggest a request to switch to Line In. | Retest. |

The separate BT UART can emit `+TITL/+ARTS/+ALBM` and `BT_PA` when the phone screen turns on or off. The firmware opens a short pending window before switching sources. If `+SRC=NONE` arrives during that window, it cancels the switch. Otherwise, it sends `MCU+PLY+PUS` to LP and selects Bluetooth. Bluetooth can also be selected directly from the menu, by IR, or through `AXX+PLM+041` from the LP app.

### App pass-through and EQ

| Direction | Command | Meaning in the PDF | Project notes |
| --- | --- | --- | --- |
| LP -> MCU | `AXX+PAS+data&` | Pass-through data from the app to the MCU. | Used for EQ. |
| MCU -> LP/App | `MCU+PAS+data&` | Pass-through data from the MCU to the app. | Used to report EQ. |
| LP -> MCU | `AXX+PAS+EQGet&` | The app queries EQ state. | The firmware responds. |
| LP -> MCU | `AXX+PAS+EQSet:bass:n&` | Sets bass EQ. | The firmware converts it to DSP bass gain. |
| LP -> MCU | `AXX+PAS+EQSet:treble:n&` | Sets treble EQ. | The firmware converts it to DSP treble gain. |
| MCU -> LP/App | `MCU+PAS+EQ:bass:n&` / `MCU+PAS+EQ:treble:n&` | MCU reply format in the PDF. | The firmware currently reports using `EQSet`. Check compatibility with the PDF format. |

## Firmware decisions and tests

1. Proposed hard-off handling.
   - Send `MCU+POW+OFF`.
   - Wait 8 s for `AXX+POW+OFF`.
   - On `AXX+POW+OFF`, blink the mute LED twice briefly to indicate that the user can unplug the unit.
   - On `AXX+BURNING`, cancel power-off.
   - On `AXX+BURNING`, blink the mute LED three times quickly, then leave it on.
   - On timeout, blink the mute LED four times, then leave it on.
   - The current PCB has no LP rail control GPIO. The handshake signals when the user can disconnect power manually.

2. Standby and power-save handling.
   - `MCU+POW+SAV` enters LP power-save and produces `AXX+POW+001` and `AXX+RA0+000`.
   - `MCU+POW+RES` returns LP to active and produces `AXX+POW+000`. It can also trigger Wi-Fi prompts and events.
   - On leaving standby, query `MCU+POW+STA` first. If it returns `000`, skip `RES`.

3. Boot handshake observations are complete. The desired product behavior still needs review.
   - `AXX+BOT+DON/UP0` appeared during cold boot.
   - The firmware replies once with `MCU+BOT+DON`.
   - Sending `MCU+BOT+DON` manually produces a `AXX+BOT+DON` acknowledgment. Gate the automatic reply with a flag.
   - After hard-off, the intended cold-boot screen sequence is `Handshake ready` -> `WiFi Connected` -> `Time synced` -> Standby.
   - The “connected to your WiFi network” announcement arrives near the end of the handshake. Show a separate `WiFi Connected` stage after it.
   - `Time synced` should be the final stage, with the progress animation on its last dot.

4. Current RTC behavior is sufficient.
   - `AXX+GET+RTC` -> `MCU+RTC+GET`.
   - If LP is not ready, it returns `AXX+RTC+NIL`.
   - When network time becomes available, LP sends `AXX+SET+RTC...`.
   - Silent parser support could be added later for `AXX+APP+RTC...` and `AXX+SET+WEK...`.

5. Test the Bluetooth source-switching commands from the PDF.
   - `AXX+BT+CON1/CON0`
   - `MCU+BT+CONN`
   - `MCU+BT+PAIR`
   - AX31 observations include `AXX+BT+GPRL`, `AXX+SIL+001` and `AXX+SIL+000`.

6. Test `lp MCU+RTC+GET` during playback.
   - Record whether it returns `AXX+SET+RTC...`, `AXX+RTC+NIL` or another response.

7. Check whether older AX31 firmware ever sends `AXX+CAP+GET`.
   - If it does, the minimum response set is `MCU+CAP+PRJ...&`, `MCU+PTV...`, `MCU+CAP+001...&`, `MCU+CAP+002...&`, `MCU+CAP+PLM...&` and `MCU+SID...&`.
   - This exchange may be important during OOBE or factory restore.

## Compatibility points to preserve

- UART baud and framing. Use the firmware's current settings for this AX31 module.
- `MCU+PLM-xxx`. The firmware and AX31 boot notes use this form.
- `MCU+POW+SAV`. Preserve the observed behavior and label it as power-save.
