# HDMI capture tools and privacy

Research date: 2026-10-07. Baseline: master `e266c144974e28981fd3d43703ae40a8a55380e3`, Wii64 1.6.18, Intel macOS 13.7.8. This review extends [capture validation](capture-device-research-2026-10-07.md) and [software research](capture-software-research-2026-10-07.md). It does not change Wii64 code.

## Required privacy boundary

The owner declined Microphone access for Codex. Do not request it or try another API to evade that decision. The owner subsequently authorized an app-wide grant for a separate CLI-backed recorder. Wii64 1.6.19 packages the existing native helper as **Wii64 HDMI Capture.app**, launched through LaunchServices. Approve that recorder only; never approve a prompt naming Codex. The prior VM proposal remains optional, not required for the accepted app-wide grant.

macOS exposes Microphone authorization per application, not a supported per-USB-device grant. A script's input ID selects a source; it does not restrict the OS grant. Running FFmpeg, Swift or Python is not an exception. The existing native helper uses Codex's responsible-app privacy context when launched here. A separately launched capture application can have its own authorization, but that remains app-wide. [Apple microphone controls](https://support.apple.com/guide/mac-help/control-access-to-the-microphone-on-mac-mchla1b1e1fe/mac), [Apple capture authorization](https://developer.apple.com/documentation/avfoundation/requesting-authorization-to-capture-and-save-media).

| Architecture | Access boundary | Limitation |
|---|---|---|
| Queued native CLI through the separate app (1.6.19) | Recorder owns Camera/Microphone grants; Codex stays denied | Recorder's grant is app-wide; actual audio still needs a hardware check |
| User-launched QuickTime or OBS | Capture app gets its own grant; Codex analyzes authorized output files | App can access other audio inputs if permitted; source selection is not OS isolation |
| Small independent recorder | Audited binary fixes the UGREEN input and refuses fallback | Application policy only; signing, launch attribution and updates need validation |
| Linux VM with only UGREEN passed through | Guest receives that composite USB device, not the Mac microphone | Hypervisor remains trusted; compatibility, throughput and host-input forwarding need testing |
| Separate Linux capture host | Dongle physically connects to a different machine | Extra hardware; transfer only the intended results |

For stronger device-specific isolation on this workstation, test the VM option before building automation. UTM supports USB sharing through its QEMU backend, but warns that some devices fail because of host configuration/reset behavior. Do not promise 60 fps or exclusive behavior without testing this dongle. Do not pass through other audio devices or enable an emulated host microphone. [UTM USB sharing](https://docs.getutm.app/guest-support/sharing/usb/), [UTM sound device configuration](https://docs.getutm.app/settings-qemu/devices/sound/).

UTM includes `utmctl` and AppleScript automation; not every operation is supported. Start with its installed help and verify USB attachment support rather than inventing commands. A CLI-controlled guest remains a VM, not a macOS permission loophole. [UTM scripting](https://docs.getutm.app/scripting/scripting/).

## Confirmed device capabilities

The connected UGREEN 15389 reports VID `0x2b89`, PID `0x5389`, manufacturer MACROSILICON and a SuperSpeed connection. These identify the device, not its exact chip or firmware. Video ID: `0x146000002b895389`; reinspect after reconnection.

Read-only inspection found:

- USB audio input: two channels, current nominal rate 48,000 Hz. No audio was recorded; supported bit depths and audio integrity remain unknown.
- Vendor HID: usage page `0xff00`, usage 1, eight-byte feature report. This does not identify a command protocol.
- Four UVC processing controls. All report minimum 0, maximum 100, step 1, default 50 and current 50: brightness, contrast, saturation and hue. Signedness differs by control. No values were changed.

Pinned `uvc-util` `8110da7025c95eea3096a7181af9a46c0cc7ac37` built with installed Apple Command Line Tools. Queue jobs `20261007-145306-8150e5` and `20261007-145506-64f8b6` completed successfully. Each subprocess had a 15-second deadline. Its list/show/get operations were inspected before use. Source and binary remain under ignored `.dev/tools/capture-research/`.

```bash
# Read-only example; run within the existing Wii queue lease.
.dev/tools/capture-research/uvc-util/uvc-util-probe \
  --select-by-vendor-and-product-id=0x2b89:0x5389 \
  --list-controls --show-control=brightness --get=brightness
```

The tool uses IOKit for standard UVC controls; these are separate from vendor HID commands. Changing controls requires a calibration experiment that records and restores original values. [Pinned uvc-util source](https://github.com/jtfrey/uvc-util/tree/8110da7025c95eea3096a7181af9a46c0cc7ac37).

## Tools worth using

| Priority | Tool | Role and restriction |
|---|---|---|
| Now | Existing native capture scripts | Proven queued video, warmup, exact device, deadlines, size cap, partial finalization and compact reports. No incoming server. |
| Now | `ioreg`, `system_profiler`, `hidutil` | Target-filtered USB/audio/HID inventory. Avoid dumping unrelated host devices. |
| Now | `uvc-util` | Proven read-only control discovery on this Mac. Do not reset/set controls during a benchmark. |
| Next | FFmpeg / ffprobe | Offline stream/PTS inspection, silence/level statistics and black/freeze observations. Confirm the exact build's filters and encoders. No audio capture launched from Codex. |
| Optional | OBS / QuickTime | User-controlled preview and independently authorized recording. OBS offers meters and resilient recording formats. QuickTime is already part of macOS. |
| Optional | Installed VLC 3.0.18 | Secondary preview/codec comparison. This unit's paired audio/video workflow was not tested. |
| Conditional | Linux `v4l2-ctl`, `arecord`, `lsusb`, `usbmon`, `edid-decode` | Formats, controls, PCM capture, descriptors, transfer tracing and offline EDID decoding on an isolated capture guest/host. |
| Conditional | Windows USBView / USBPcap | Descriptor and transfer reference checks on a separate Windows capture host. Do not replace working capture drivers. |

See the software note for encoder, storage, license and compatibility details. FFmpeg is not installed here. Hardware encoding must be confirmed at runtime; an H.264 file does not establish the USB transport encoding. [FFprobe](https://ffmpeg.org/ffprobe.html), [VideoToolbox](https://developer.apple.com/documentation/videotoolbox).

Linux tools expose a different driver path, not a guaranteed improvement. Optional UVC metadata can carry device timing, but is not proof of end-to-end latency. USB traces describe transfers, not individual wire transactions. Do not weaken SIP to obtain macOS traces. [v4l-utils](https://github.com/gjasny/v4l-utils), [UVC metadata](https://www.kernel.org/doc/html/v6.0/userspace-api/media/v4l/pixfmt-meta-uvc.html), [USB capture constraints](https://wiki.wireshark.org/CaptureSetup/USB), [Windows USBView](https://learn.microsoft.com/en-us/windows-hardware/drivers/debugger/usbview).

## Vendor tooling: investigate, do not flash

`ms-tools` is the relevant MacroSilicon HID research project, not a safe generic settings utility. At pinned commit `967bb582c20cbe33523f3484d0aaf32801d3a4d9`, non-list commands initialize its HAL with patch installation enabled by default. ROM dumping can upload executable code; chip identification includes guesses. Even apparent reads can configure hardware. Our VID/PID differs from its defaults. It was cloned for source review, not built or run. [Pinned source](https://github.com/BertoldVdb/ms-tools/tree/967bb582c20cbe33523f3484d0aaf32801d3a4d9).

Do not issue guessed HID reports, upload patches, edit EDID or flash this unit. First prove chip/protocol compatibility and a recovery path. Vendor HID existence is insufficient evidence.

Notable alternatives reviewed:

- `ms213x-rename` edits firmware/EDID files offline; it is not a live capture controller. [Source](https://github.com/starainrt/ms213x-rename).
- `macrosilicon_firmware` advertises specific older chips, not this unidentified SuperSpeed unit. Do not transplant its firmware. [Supported targets](https://github.com/kraln/macrosilicon_firmware).
- `libuvc`/libusb could support a separate Linux experiment; replacing the proven Mac driver adds ownership/conflict risk. [libuvc](https://github.com/libuvc/libuvc).
- `camtint`/`uvcctl` is a source reference, not the preferred deployment: its advertised Mac version is newer, it adds a UI/server, and its source can seize a USB device. [Source](https://github.com/bornaware/camtint).
- USBProberV2-Redux requires macOS 15 or later; this Mac runs 13. It decodes descriptors, not raw USB packets. [Requirements](https://github.com/gingerbeardman/USBProberV2-Redux).

## Measurement plan

1. Validate the chosen privacy boundary with a ten-second capture. Keep Codex Microphone denied. For a VM, verify only UGREEN is exposed, no host audio input is forwarded, and actual video/audio streams and timing are usable. Stop rather than adding broader permission if it fails.
2. Use a queued 240p Test Suite run for known patterns and audio/video synchronization. Check the entire Wii-to-HDMI-to-USB chain before blaming Wii64. Save original control values; calibrate only in a separate experiment. [240p Test Suite](https://github.com/ArtemioUrbina/240pTestSuite).
3. Reuse bounded captures and offline reports: timing percentiles, repeated-frame observations, brightness, audio peaks/RMS, silence and synchronization markers. Flag findings for review; static menus, fades and deliberate silence are not failures.
4. Compare capture-on/off host collection behavior and use identical capture settings across A/B/A runs. Delivered USB frames can repeat a guest frame. Use Wii64 probes as the performance authority, not capture FPS.

The original research made no VM, package, firmware or audio-permission changes. With the subsequent CLI-recorder approval, the next experiment is a bounded queued HDMI audio/video capture through the separate app, followed by a permission-state comparison with the direct Codex-launched helper. Keep media, descriptors with personal identifiers and firmware dumps out of Git.

## Local-model review

Two asynchronous courts used `muse-glimmer:30b` and `gemma4:26b`, each with a 10,000-token response cap; both completed. They helped surface privacy and vendor-tool risks. Source checks rejected claims that USB passthrough provides IOMMU isolation, that transferring intentionally captured files defeats the privacy goal, or that an encoded video codec identifies USB transport. A fetch/summarize job hit a 1,400-token cap; its incomplete answer was not used as evidence. Improvements: expose an explicit fetch budget, flag truncation clearly, and separate supplied facts from platform-specific hypotheses.

## CLI-backed recorder update (1.6.19)

The owner accepted app-wide permission for a separate recorder. The existing
Swift helper now runs inside a signed app bundle for live commands. `open`
launches it through LaunchServices; private temporary logs and a PID/completion
file preserve its actual result. Missing completion after a crash fails the
command. Timeout/cancellation verifies the PID's executable before signaling it.
Queued captures freeze the complete signed bundle and launcher. No new server,
login item, VM or media dependency was added.

Validation:

- Native CLI: 10 tests pass. Runner: 19 tests pass. Build-cache checks pass,
  including unchanged-app reuse, tampering and failed-build preservation.
- The full host/subsystem suite passes. The final clean glN64 Wii build passes
  with documented HBC SDK 1.9.4, commit `0b214c7a`, built into Wii64's own archive.
  The shared HBC checkout stays at 1.8.8 and was not changed. SDK source snapshot:
  `/private/tmp/wii64-capture-sdk.nggTth`; its sparse checkout includes `sdk` and
  `channel/channelapp`, which the SDK makefile requires. Pass that directory as
  `HBC_AGENT_ROOT` for subsequent builds while the shared source remains older.
- Muted Dolphin startup of the final build passes in a separate profile, with
  no invalid-access, DSP or SD-sync warnings. DOL SHA-256:
  `fe694e36d8f34fd146e550317728c95a1423040d9ac57e0cefe59a2999bc8242`.
- macOS TCC logs identify the Microphone request's subject and responsible
  process as `org.wii64.dev-capture`, not Codex. The recorder initially reports
  `not_determined`; the direct Codex-launched helper still reports Microphone
  denied. These checks do not record an audio input.
- Both authorization attempts timed out without a decision; the second used a
  three-minute response window. HDMI audio recording and copied-bundle grant
  persistence remain pending until approval succeeds. Do not label these passed.
- A real LaunchServices invocation of an invalid recorder command returns the
  recorder's exit code 2 rather than a launch-success result. macOS can warn that
  a very short-lived app exited before `open` could wait; the completion marker
  still preserves the actual failure in this check.

The hardware smoke test accepts `--audio-device-id ID`, checks an encoded audio
track in every tested mode, and retains cancellation/file-cap checks. Use it
under the existing Wii lease after approval; it records the existing HDMI
picture and leaves the console untouched. A track proves recording, not sound
quality or correct Wii64 synthesis.

Two further local-model courts used 10,000-token caps and completed in roughly
46 and 90 seconds. Their launch/status cautions informed the implementation.
Their unsupported code findings were rejected against live `open -h`, source
and tests, including claims that this Mac lacks stdout/argument flags and that
the app cache stores the unbundled executable's hash. Local review needs better
adherence to supplied positive evidence; no production change came from those
false findings.
