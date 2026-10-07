# Intel macOS capture research: UGREEN 15389

Research date: 2026-10-07. This section is an API and source review, not a live test of the device. I found no first-party UGREEN source that specifies this unit’s exact modes. Use runtime discovery instead of a similar product's specifications.

## Safe recommendations

1. **Enumerate the connected device at runtime.** In AVFoundation, identify the video `AVCaptureDevice`, inspect `formats`, each format description’s dimensions/subtype, and each `videoSupportedFrameRateRanges` entry. Select only a mode actually listed, then confirm the active format and observed sample-buffer timestamps after starting the session. Apple defines these properties as the device’s supported formats and frame-rate ranges. [Apple: capture formats](https://developer.apple.com/documentation/avfoundation/capture-device-formats) · [Apple: `AVCaptureDevice.Format`](https://developer.apple.com/documentation/avfoundation/avcapturedevice/format)

2. **Separate input mode from decoded pixel format.** `AVCaptureVideoDataOutput.availableVideoPixelFormatTypes` describes accepted output formats, not necessarily the USB wire encoding. Configure the advertised device mode and supported decoded output type separately. [Apple: output pixel formats](https://developer.apple.com/documentation/avfoundation/avcapturevideodataoutput/availablevideopixelformattypes)

   **Practical implication:** requesting NV12 does not prove NV12 USB transport. Use compressed MJPEG only if the device and API expose it. UVC defines MJPEG and uncompressed payload formats, but does not specify this product's descriptors. [USB-IF: UVC 1.5 document set](https://www.usb.org/document-library/video-class-v15-document-set)

3. **For FFmpeg discovery, enumerate device IDs and audio separately.** `ffmpeg -f avfoundation -list_devices true -i ""` lists AVFoundation video/audio devices, names, indices, unique IDs and available USB serials. FFmpeg documents that a UVC video unique ID may change across reboots; when available, its `serial:` selector is more stable. AVFoundation’s video and audio inputs are separate devices/streams, so select the HDMI audio capture endpoint explicitly if it appears in the audio list—do not infer that its name/index matches the video device. [FFmpeg AVFoundation input docs](https://ffmpeg.org/ffmpeg-devices.html#avfoundation) · [Apple: capture session inputs](https://developer.apple.com/documentation/avfoundation/setting-up-a-capture-session)

4. **Avoid competing capture clients and distinguish capture ownership from configuration locks.** Keep one app/session responsible for the video device while recording. AVFoundation exposes `isInUseByAnotherApplication` and a session interruption reason for a video device used by another capture session. `lockForConfiguration()` requests exclusive access to configure device properties; it is not documented as a general-purpose reservation for an entire recording. Observe running/interrupted/runtime-error state and report the specific failure rather than repeatedly retrying. [Apple: `AVCaptureDevice`](https://developer.apple.com/documentation/avfoundation/avcapturedevice) · [Apple: session interruption reasons](https://developer.apple.com/documentation/avfoundation/avcapturesession/interruptionreason)

5. **Measure startup behavior and timing.** The API contract gives no device-specific preroll count. Record a short diagnostic with a known HDMI picture and inspect early frames. Do not discard every black frame: black may be source content. Record presentation timestamps, dimensions, intervals, dropped-frame callbacks and session errors. [Apple: sample-buffer presentation timestamp](https://developer.apple.com/documentation/coremedia/cmsamplebuffer/presentationtimestamp) · [Apple: dropped video frames](https://developer.apple.com/documentation/avfoundation/avcapturevideodataoutputsamplebufferdelegate/captureoutput%28_%3AdidDrop%3Afrom%3A%29)

6. **Query controls; do not probe by changing them.** Apple documents capability checks for individual focus, exposure and other camera properties, and requires configuration locking before supported properties are changed. Those APIs do not establish that a particular HDMI capture device implements those controls. UVC class-control and extension-unit capabilities are device-descriptor-specific; macOS AVFoundation documentation reviewed here does not promise a generic interface for setting arbitrary UVC extension-unit controls. Read reported capabilities only, and leave controls untouched. [Apple: capture-device configuration and supported controls](https://developer.apple.com/documentation/avfoundation/avcapturedevice) · [USB-IF: UVC 1.5 document set](https://www.usb.org/documents?items_per_page=50&order=title&search=u+s+b&sort=desc)

## What cannot be assumed for this unit

- Exact resolutions, frame rates, frame-rate intervals, pixel subtypes, or simultaneous format/audio combinations. Enumerate them on the target Mac; no 4K/60, 1080p/60, or other marketed/third-party specification is verified here.
- MJPEG, YUY2/YUYV, or NV12 as the device’s USB transport mode. NV12 and YUYV422 may be AVFoundation output formats, which is a distinct fact from UVC bus encoding. “YUY2” is commonly used as a FourCC spelling; confirm the actual subtype/output listing rather than treating the names as interchangeable proof.
- A fixed number of black startup frames, signal-lock delay, or automatic black-frame behavior.
- An exclusive-device policy identical across macOS versions/apps. Another client can make the device unavailable or interrupt a session; actual arbitration behavior should be observed on the deployed macOS version.
- HDMI audio presence, audio endpoint name, serial association with the video endpoint, or stable enumeration index.
- UVC brightness/contrast/saturation controls, extension units, firmware behavior, or any hidden vendor feature. No firmware behavior is inferred or claimed.

## Copyright handling

Keep copyrighted captures ignored and out of version control; verify the output path is covered by an ignore rule before recording. This research did not create or inspect any capture files.

## Primary sources

- [Apple AVFoundation capture formats](https://developer.apple.com/documentation/avfoundation/capture-device-formats)
- [Apple `AVCaptureVideoDataOutput.availableVideoPixelFormatTypes`](https://developer.apple.com/documentation/avfoundation/avcapturevideodataoutput/availablevideopixelformattypes)
- [Apple capture-session setup](https://developer.apple.com/documentation/avfoundation/setting-up-a-capture-session)
- [Apple `AVCaptureDevice`](https://developer.apple.com/documentation/avfoundation/avcapturedevice)
- [Apple `CMSampleBuffer.presentationTimeStamp`](https://developer.apple.com/documentation/coremedia/cmsamplebuffer/presentationtimestamp)
- [Apple video data output drop callback](https://developer.apple.com/documentation/avfoundation/avcapturevideodataoutputsamplebufferdelegate/captureoutput%28_%3AdidDrop%3Afrom%3A%29)
- [USB-IF document library, UVC 1.5](https://www.usb.org/documents?items_per_page=50&order=title&search=u+s+b)
- [FFmpeg AVFoundation device documentation](https://ffmpeg.org/ffmpeg-devices.html#avfoundation)
- [FFmpeg AVFoundation source](https://github.com/FFmpeg/FFmpeg/blob/master/libavdevice/avfoundation.m)

## Live validation on this Intel Mac

Wii64 1.6.18 capture tooling; macOS 13.7.8, Swift 5.9.2. No emulator engine
changes. Reference: WiiXplorer-NG `e85f62851547c0e7dbf74897234eafc1c3621bac`,
`scripts/wii-capture.m`, `scripts/build-wii-capture.py`, and `DEBUGGING.md`.
The reference's frame 0 was black; later frames showed UGREEN startup graphics,
then WiiXplorer and HBC. The old Wii64 first-frame snapshot was entirely black.
The new warmup snapshot and periodic frames show HBC. Dark-frame analysis remains
an observation, not an automatic signal-loss diagnosis.

USB inventory reports MACROSILICON, VID `0x2b89`, PID `0x5389`, SuperSpeed,
900 mA available and 512 mA requested. Those are descriptor reports, not measured
current consumption or proof of a particular chip/firmware. AVFoundation lists
22 video formats: paired `yuvs`/`420v` modes from 720×480 to 2560×1440.
The highest advertised rate is about 60 fps through 1920×1080, and about 30 fps
at 2560×1440. Decoded outputs include `2vuy`, `yuvs`, `420v`, `420f`, `ARGB`, `BGRA`.
The audio endpoint is named UGREEN 15389; Microphone access is denied, so audio
recording remains unvalidated. Focus, exposure and white balance report unsupported.
Zoom, exposure bias and arbitrary vendor extensions are not exposed by this Mac API.

Queue job `20261007-100942-760074` ran the reusable hardware test against the HBC
picture. Results are capture rates, not guest N64 speed. Movies include preroll.

| Input / encoded movie | Delivered fps | Callback drops | Encoded duration | Result |
|---|---:|---:|---:|---|
| 1280×720 / H.264 | 60.03 | 0 | 5.048 s | pass |
| 1920×1080 / H.264 | 59.37 | 0 | 5.049 s | pass |
| 2560×1440 / H.264 | 29.64 | 0 | 5.099 s | pass |
| SIGTERM at first periodic frame | — | 0 | 2.049 s | partial movie finalized; cancellation did not pass as complete |
| 1 MiB file cap | — | — | partial | early stop correctly failed the capture |

Three-second capture windows used three-second preroll. The test checked actual
encoded dimensions and video tracks, not only selected device properties. Input
720×480 frames/JPEGs also worked at about 60 fps, but this MOV path repeatedly
returned `Cannot Record` at both 30 and 60 fps. This is unresolved; do not call it
a proven hardware limit. Use 720p60 for routine recording.

Measured implementation fixes:

- Pump the main run loop; do not wait on a main-thread semaphore for delegate work.
- Start preroll from the first delivered frame, not compiler/session setup time.
- Clamp rational frame durations within the advertised range. Microsecond
  truncation initially requested 60.002399 fps and raised an Objective-C exception.
- Session startup reset explicit resolution and minimum frame duration. Apply
  the selected format/rate after startup, and verify delivered frames.
- Set H.264 output dimensions before startup: changing encoder settings afterward
  renegotiated the input to 1080p. Inspect actual encoded tracks after finalization.
- Include preroll in movies; report real encoded duration. Encoder startup consumed
  about 0.9 seconds in the early short-clip trials.
- Keep only cheap timestamps/dimensions per frame; sample brightness/JPEGs at the
  configured interval. Callback-drop counts do not prove there were no driver gaps.

The complete host/subsystem suite, clean glN64 Wii build, and muted fresh-profile
Dolphin menu smoke passed. The older Dolphin profile's SD-sync backup was preserved.
All movies, images, per-frame JSON and logs stay in ignored `.dev/runs/`.

## Lower-level access: not part of routine capture

[ms-tools](https://github.com/BertoldVdb/ms-tools) documents MacroSilicon HID access
to GPIO, I2C and firmware memory. Its HAL can patch running firmware; some operations
that read code upload code first. That is not a harmless device-inventory command.
This unit's chip/firmware compatibility is not established. No HID patches, firmware
flashes, EEPROM writes, EDID changes or USB-driver replacements were made. A future
firmware experiment needs verified hardware identity, a backup and a recovery plan.

For routine discovery use `usb-info` and `inspect`; neither installs a driver,
claims a USB interface, or changes OS privacy/firewall policy. Use the standard
[Apple output-settings API](https://developer.apple.com/documentation/avfoundation/avcapturemoviefileoutput/setoutputsettings(_:for:))
for host encoding; device input, decoded pixels and H.264 file encoding are distinct.
