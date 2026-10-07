# macOS HDMI capture software alternatives

Research date: 2026-10-07. Target host: Intel Mac, macOS 13.7.8. Context supplied by the project: its native capture path already records queued MOV/JPEG output, with 720p60, 1080p60, and 1440p30 reported verified. This note compares OBS, FFmpeg, VLC, and mpv as host software alternatives; it does not revisit device firmware or re-verify those project results.

**Boundary:** macOS AVFoundation and the applications below provide host capture paths. Their documentation does not establish which modes the UGREEN 15389 / MacroSilicon VID `2b89` PID `5389` actually exposes, whether its audio endpoint appears to macOS, or whether any mode works in these applications. Device-reported modes and host APIs are separate facts. No hardware was accessed, no app was installed, and none of the commands below were run.

**Microphone privacy constraint:** Do not grant or request Microphone permission to Codex or launch native Mac audio capture here. This project's native CLI uses Codex's responsible-app context. FFmpeg is not a permission escape; its launch attribution has not been tested here. A separately trusted app can be authorized independently, but device selection does not narrow its app-wide grant. See [capture tools and privacy](capture-tools-and-privacy-2026-10-07.md). [Apple microphone controls](https://support.apple.com/guide/mac-help/control-access-to-the-microphone-on-mac-mchla1b1e1fe/mac).

## At a glance

| Tool | macOS 13 Intel status | Capture and audio role | Practical fit |
|---|---|---|---|
| OBS Studio | Strong: current compatibility table lists OBS 32.2.1 for Intel on Ventura and later | Video Capture Device source for capture cards; separate audio-device source; macOS 13+ desktop/app audio capture | Best all-in-one preview, scene, audio-meter, and recording workflow. Intel VideoToolbox recording is H.264; OBS says its Intel VT path is not suitable for streaming. |
| FFmpeg | Strong API support; executable support depends on the chosen build and its enabled components | AVFoundation input enumerates and opens video/audio devices separately; filters include audio level/statistics analysis | Best for scripted, repeatable direct capture, explicit stream mapping, post-capture analysis, and transcoding. |
| VLC | OS and architecture are supported by current Intel download; its macOS source has AVFoundation capture modules | GUI capture-device path; macOS video uses AVFoundation. Audio capture exists as a separate macOS AVFoundation module, but a particular device pairing is not established here. | Useful GUI preview/record/conversion alternative; fewer documented capture and analysis controls than FFmpeg or OBS. |
| mpv | Project minimum is macOS 10.15; available on Ventura, but build features vary | `av://` can pass an FFmpeg `libavdevice` input; encoding mode can write output | Viable lightweight preview or a constrained FFmpeg-backed pipeline when the build includes AVFoundation. Not a turnkey capture/monitoring workflow. |

Sources: [OBS macOS versions](https://obsproject.com/kb/macos-versions), [OBS capture sources](https://obsproject.com/kb/video-capture-sources), [OBS audio capture guide](https://obsproject.com/kb/macos-desktop-audio-capture-guide), [FFmpeg AVFoundation input](https://ffmpeg.org/ffmpeg-devices.html#avfoundation), [VLC macOS download/requirements](https://images.videolan.org/vlc/download-macosx.en_GB.html), [VLC AVFoundation video source](https://github.com/videolan/vlc/blob/master/modules/access/avcapture.m), [VLC AVFoundation audio source](https://github.com/videolan/vlc/blob/master/modules/access/avaudiocapture.m), [mpv installation](https://mpv.io/installation/), [mpv manual](https://mpv.io/manual/master/).

## Capture APIs and per-tool findings

Apple describes `AVCaptureSession` as the macOS capture-session foundation, with capture inputs and outputs connected by the host app. FFmpeg’s `avfoundation` input is a user-facing route to this host API: it lists device names, indices, unique IDs, and sometimes USB serials; it can select video and audio independently and request a size, frame rate, or pixel format. The requested settings must still be accepted by the device and host stack. A UVC unique ID may change across reboots; FFmpeg documents `serial:` selection only when the device exposes a serial. [Apple capture-session setup](https://developer.apple.com/documentation/avfoundation/setting-up-a-capture-session), [FFmpeg AVFoundation options](https://ffmpeg.org/ffmpeg-devices.html#avfoundation).

**OBS Studio.** OBS documents its Video Capture Device source for webcams and capture cards on macOS. On Ventura, use that source for the HDMI adapter; the separate macOS Screen Capture source captures the Mac display, window, or app, not the external adapter’s video. macOS 13+ supports desktop or per-app audio through OBS 30+ macOS Audio Capture/Screen Capture sources. Those system-audio sources are distinct from the adapter’s HDMI audio. Selecting the adapter as an audio input, if exposed, still requires the trusted OBS app to have macOS Microphone permission; that permission is app-wide, not narrowed to the selected device. OBS lists Camera permission for capture cards, Microphone permission for audio devices, and Screen Recording permission for desktop/app capture. [OBS video capture sources](https://obsproject.com/kb/video-capture-sources), [OBS screen capture source](https://obsproject.com/kb/macos-screen-capture-source), [OBS desktop audio guide](https://obsproject.com/kb/macos-desktop-audio-capture-guide), [OBS macOS permissions](https://obsproject.com/kb/macos-permissions-guide), [Apple app-specific capture authorization](https://developer.apple.com/documentation/avfoundation/requesting-authorization-to-capture-and-save-media).

For recording on Intel Macs, OBS documents Apple VideoToolbox H.264 recording; it specifically says Intel VT is not supported for streaming because of constant-bitrate limitations. x264 software encoding remains an option, but CPU cost depends on resolution, frame rate, and preset. The OBS Intel/macOS support table and the encoder documentation are the relevant compatibility evidence; neither predicts a given Mac’s sustained 1440p30 performance. [OBS hardware encoding](https://obsproject.com/kb/hardware-encoding), [OBS macOS versions](https://obsproject.com/kb/macos-versions).

**FFmpeg.** Video and audio indices are separate lists. Under this project's privacy constraint, use it here for offline analysis, not Mac audio capture. Upstream supplies source; packaged executables can differ in architecture, minimum macOS target and enabled components. Check `ffmpeg -devices`, `ffmpeg -encoders` and `ffmpeg -filters` in the exact build. [FFmpeg input options](https://ffmpeg.org/ffmpeg-devices.html#avfoundation), [FFmpeg build distribution](https://www.ffmpeg.org/download.html).

**VLC.** VideoLAN’s desktop guide documents opening an acquisition card/capture device and selecting device names; its Mac recording instructions select an input device and video checkbox. VLC’s source identifies `avcapture.m` as the macOS AVFoundation video module; current source also has a separate AVFoundation audio-capture module. The VLC 3.0 release notes record replacing the old macOS qtsound capture module with AVFoundation audio capture. This proves the software path exists in the project, not that every packaged build exposes a paired audio/video device or that this adapter is recognized. VLC’s current download page offers an Intel build and says 64-bit Intel is supported; it lists macOS 10.7.5+ as the broad requirement. [VLC capture-device guide](https://docs.videolan.me/vlc-user/desktop/3.0/en/basic/media.html), [VLC record-video guide](https://docs.videolan.me/vlc-user/desktop/3.0/en/basic/recording/video.html), [VLC video module source](https://github.com/videolan/vlc/blob/master/modules/access/avcapture.m), [VLC audio module source](https://github.com/videolan/vlc/blob/master/modules/access/avaudiocapture.m), [VLC 3.0 release notes](https://github.com/videolan/vlc-3.0/blob/master/NEWS), [VLC Intel/macOS downloads](https://images.videolan.org/vlc/download-macosx.en_GB.html). VLC’s documentation supports playback, recording, and conversion, but does not describe an equivalent of FFmpeg’s statistics filters or OBS’s mixer meters; use a dedicated audio-analysis path when measurement matters.

**mpv.** The manual defines `av://type:options` as a route to FFmpeg `libavdevice` inputs. Therefore `av://avfoundation:...` is a plausible Mac preview route only when that particular mpv build includes FFmpeg’s AVFoundation input. mpv also has encoding mode (`--o`, `--ovc`, `--oac`), but it is a media player whose direct capture workflow depends on the bundled FFmpeg build and parsing of AVFoundation options. Prefer OBS or FFmpeg for unattended/long recordings. The project’s installation page labels macOS binary packages unofficial third-party builds except its CI builds; verify the exact Intel architecture and feature list. [mpv manual: device inputs and encoding](https://mpv.io/manual/master/), [mpv installation/builds](https://mpv.io/installation/), [mpv README and platform requirements](https://github.com/mpv-player/mpv/blob/master/README.md).

## Audio routing and analysis

- For adapter audio, app-level device selection and macOS authorization are separate: a trusted app must be authorized for Microphone/audio-input access, and a selected device ID does not scope that grant. OBS or QuickTime can be authorized independently from Codex in System Settings; that remains app-wide for audio inputs. Do not request access for Codex or use Codex-launched audio capture under this project’s constraint. The listed FFmpeg recording examples are offline templates only. Capturing the Mac’s desktop/app sound is a different permission and path: OBS 30+ on macOS 13 can capture it through ScreenCaptureKit; that does not establish access to the HDMI adapter’s USB audio input. [Apple app-specific capture authorization](https://developer.apple.com/documentation/avfoundation/requesting-authorization-to-capture-and-save-media), [Apple external audio-input privacy guidance](https://support.apple.com/en-gb/102071), [FFmpeg device list](https://ffmpeg.org/ffmpeg-devices.html#avfoundation), [OBS audio input sources](https://obsproject.com/kb/audio-sources), [OBS desktop audio guide](https://obsproject.com/kb/macos-desktop-audio-capture-guide).
- OBS’s mixer gives live level meters and scene-level routing; its documentation cautions against capturing the same device both globally and as a scene source because that can create echo/duplication. [OBS audio sources](https://obsproject.com/kb/audio-sources).
- FFmpeg’s `astats` reports per-channel peak, RMS, noise, and related statistics; `volumedetect` reports mean and maximum levels at end of input; `ebur128` measures EBU R128 loudness. These are analysis filters, not proof of clean/synchronized source audio. [FFmpeg astats](https://ffmpeg.org/ffmpeg-filters.html#astats), [volumedetect](https://ffmpeg.org/ffmpeg-filters.html#volumedetect), [ebur128](https://ffmpeg.org/ffmpeg-filters.html#ebur128).
- VLC and mpv can play/monitor the feed; the primary docs reviewed here do not provide a comparable capture-oriented audio-statistics workflow. A VLC/mpv preview is not itself a recording or audio-level validation.

## Compression and storage choices

| Goal | Starting point | Trade-off |
|---|---|---|
| Compact routine recording | H.264 video + AAC audio in MKV or a resilient OBS Hybrid MP4/MOV | Small, broadly playable; lossy. OBS documents H.264 support on Intel Macs. |
| Editing master | ProRes in MOV (OBS Hybrid MOV on OBS 32+) or ProRes in MKV where the tool supports it | Larger files and higher write bandwidth; ProRes is editing-friendly. FFmpeg has software ProRes encoders; do not assume Intel hardware ProRes encoding. |
| Easy crash recovery | MKV in OBS; remux after capture if an editor requires MOV/MP4 | OBS documents MKV as recoverable before finalization; some editors need remuxing. |
| OBS-compatible recoverable MOV/MP4 | Hybrid MOV or Hybrid MP4 | OBS writes fragmented data then finalizes it to a regular compatible file; OBS 32.0 added Hybrid MOV, and its codec table allows ProRes in Hybrid MOV. |

OBS warns that conventional MP4/MOV can be unusable if finalization is interrupted; MKV and fragmented/hybrid formats are more recoverable. Hybrid MOV is the macOS default in recent OBS, supports ProRes, H.264/H.265 and PCM; Hybrid MP4 supports H.264/H.265 and AAC/ALAC/FLAC/Opus/PCM per OBS’s table. Container choice does not guarantee a codec is present in a particular build. [OBS format guide](https://obsproject.com/kb/audio-video-formats-guide), [OBS Hybrid MP4/MOV details](https://obsproject.com/kb/hybrid-mp4), [FFmpeg ProRes encoders](https://ffmpeg.org/ffmpeg-codecs.html#prores).

Storage scales with bitrate. Decimal planning estimates: 10 Mb/s video + 192 kb/s audio is about 4.6 GB/hour; 20 Mb/s + 192 kb/s is about 9.1 GB/hour, before container overhead. These are arithmetic budgets, not quality guarantees. For uncompressed 8-bit 4:2:2 frames, 1920×1080×60×2 bytes is about 0.90 TB/hour and 2560×1440×30×2 bytes about 0.80 TB/hour. Actual capture pixel formats and padded row strides change raw storage needs; compressed ProRes/H.264 file size depends on profile/content/settings. Keep free space and sustained disk throughput in mind for high-bitrate or lossless masters.

## Safe, untested examples

These examples were not run. Recording templates are for a separately authorized workflow only; do not launch Mac audio capture from Codex. The analysis commands read existing files and need no device access. Substitute verified indices; `none` selects no audio. Recordings are bounded to 30 seconds, refuse overwrite (`-n`) and use ignored `.dev/` paths. Validate input modes and encoder availability first.

```sh
# List AVFoundation video/audio endpoints and IDs. Listing exits without recording.
ffmpeg -f avfoundation -list_devices true -i ""

# Capture a selected device pair to an editing-oriented MOV; confirm encoder availability first.
ffmpeg -f avfoundation -framerate 60 -video_size 1920x1080 \
  -i "VIDEO_INDEX:AUDIO_INDEX" -t 30 -c:v prores_ks -profile:v 2 \
  -c:a pcm_s16le -n .dev/ffmpeg-capture-prores.mov

# Compact H.264/AAC recording; libx264 must exist in this FFmpeg build.
ffmpeg -f avfoundation -framerate 60 -video_size 1920x1080 \
  -i "VIDEO_INDEX:AUDIO_INDEX" -t 30 -c:v libx264 -preset veryfast -crf 20 \
  -c:a aac -b:a 192k -n .dev/ffmpeg-capture-h264.mkv

# Analyze an existing recording's first audio stream; no output file is written.
ffmpeg -i .dev/existing-capture.mkv -t 30 -map 0:a:0 -af astats=metadata=1:reset=1 -f null -
ffmpeg -i .dev/existing-capture.mkv -t 30 -map 0:a:0 -af ebur128 -f null -

# mpv preview template (requires a build with libavdevice's avfoundation demuxer).
mpv --length=30 'av://avfoundation:VIDEO_INDEX:none'
```

FFmpeg’s AVFoundation syntax and codec/filter availability are build-dependent; `prores_ks` and `libx264` are alternatives, not guaranteed encoders. [FFmpeg device docs](https://ffmpeg.org/ffmpeg-devices.html#avfoundation), [FFmpeg codecs](https://ffmpeg.org/ffmpeg-codecs.html), [FFmpeg filters](https://ffmpeg.org/ffmpeg-filters.html), [mpv device input syntax](https://mpv.io/manual/master/).

## Licensing and distribution notes

| Project | Upstream license summary |
|---|---|
| OBS Studio | GPL-2.0-or-later. |
| FFmpeg | LGPL-2.1-or-later by default; enabling optional GPL components makes the FFmpeg build GPL. Check the exact binary/build configuration. |
| VLC | VLC is GPLv2-or-later overall; individual source files/modules can carry other licenses, including LGPL. Check the relevant component and distribution. |
| mpv | GPLv2-or-later by default; an LGPLv2.1-or-later mode is documented for builds excluding GPL-only source files, but linked dependencies (including FFmpeg) also affect the result. |

These licenses matter most if redistributing binaries or incorporating libraries; they do not change capture compatibility. [OBS project license](https://github.com/obsproject/obs-studio), [FFmpeg legal FAQ](https://ffmpeg.org/legal.html), [VLC project](https://github.com/videolan/vlc), [VLC AVFoundation module license header](https://github.com/videolan/vlc/blob/master/modules/access/avcapture.m), [mpv copyright/license details](https://github.com/mpv-player/mpv/blob/master/Copyright).

## Primary sources reviewed

Apple: [AVFoundation capture-session setup](https://developer.apple.com/documentation/avfoundation/setting-up-a-capture-session); [app-specific capture authorization](https://developer.apple.com/documentation/avfoundation/requesting-authorization-to-capture-and-save-media); [microphone access controls](https://support.apple.com/en-my/guide/mac-help/mchla1b1e1fe/mac); [external audio-input permission guidance](https://support.apple.com/en-gb/102071); [QuickTime audio-input selection](https://support.apple.com/guide/quicktime-player/qtpf25d6f827/10.5/mac/27); [ScreenCaptureKit](https://developer.apple.com/documentation/screencapturekit).

OBS: [macOS version matrix](https://obsproject.com/kb/macos-versions); [system requirements](https://obsproject.com/kb/system-requirements); [capture-device source](https://obsproject.com/kb/video-capture-sources); [audio sources](https://obsproject.com/kb/audio-sources); [permissions](https://obsproject.com/kb/macos-permissions-guide); [desktop audio](https://obsproject.com/kb/macos-desktop-audio-capture-guide); [hardware encoding](https://obsproject.com/kb/hardware-encoding); [recording formats](https://obsproject.com/kb/audio-video-formats-guide); [hybrid containers](https://obsproject.com/kb/hybrid-mp4).

FFmpeg: [device input docs](https://ffmpeg.org/ffmpeg-devices.html); [codec docs](https://ffmpeg.org/ffmpeg-codecs.html); [filter docs](https://ffmpeg.org/ffmpeg-filters.html); [licensing](https://ffmpeg.org/legal.html); [download/build distribution note](https://www.ffmpeg.org/download.html).

VideoLAN: [capture-device docs](https://docs.videolan.me/vlc-user/desktop/3.0/en/basic/media.html); [recording docs](https://docs.videolan.me/vlc-user/desktop/3.0/en/basic/recording/video.html); [macOS Intel requirements/downloads](https://images.videolan.org/vlc/download-macosx.en_GB.html); [AVFoundation video source](https://github.com/videolan/vlc/blob/master/modules/access/avcapture.m); [AVFoundation audio source](https://github.com/videolan/vlc/blob/master/modules/access/avaudiocapture.m); [3.0 release notes](https://github.com/videolan/vlc-3.0/blob/master/NEWS); [project/license](https://github.com/videolan/vlc).

mpv: [manual](https://mpv.io/manual/master/); [installation/builds](https://mpv.io/installation/); [platform README](https://github.com/mpv-player/mpv/blob/master/README.md); [license details](https://github.com/mpv-player/mpv/blob/master/Copyright).
