import AVFoundation
import AppKit
import CoreImage
import Foundation

private let defaultDevice = "UGREEN 15389"
private var launchStatusURL: URL?
private var launchExitCode: Int32 = 1

private func writeLaunchStatus(_ code: Int32?) {
    guard let url = launchStatusURL else { return }
    let value: [String: Any] = ["pid": ProcessInfo.processInfo.processIdentifier,
                              "exitCode": code.map { Int($0) as Any } ?? NSNull()]
    do { try JSONSerialization.data(withJSONObject: value).write(to: url, options: .atomic) }
    catch { fputs("wii_video: cannot write launcher status: \(error)\n", stderr) }
}
private func finish(_ code: Int32) -> Never {
    launchExitCode = code
    exit(code)
}

private func fail(_ message: String, code: Int32 = 1) -> Never {
    fputs("wii_video: \(message)\n", stderr)
    finish(code)
}

private func usage() -> Never {
    fail("usage: wii_video {list|authorize [--audio]|inspect [--device NAME|--device-id ID]|analyze <jpg> [--require-nonblack]|self-test|snapshot <jpg> [options]|capture <mov> [options]}", code: 2)
}

private struct ImageStats {
    let meanBrightness: Double
    let peakBrightness: Double
    let darkFraction: Double
    var essentiallyBlack: Bool { peakBrightness < 0.04 || (meanBrightness < 0.025 && darkFraction > 0.98) }
}

private func imageStats(_ rep: NSBitmapImageRep, grid: Int = 16) -> ImageStats {
    let width = rep.pixelsWide, height = rep.pixelsHigh
    guard width > 0, height > 0 else { return ImageStats(meanBrightness: 0, peakBrightness: 0, darkFraction: 1) }
    var sum = 0.0, peak = 0.0, dark = 0, count = 0
    for gy in 0..<grid { for gx in 0..<grid {
        let x = min(width - 1, (gx * 2 + 1) * width / (grid * 2))
        let y = min(height - 1, (gy * 2 + 1) * height / (grid * 2))
        guard let color = rep.colorAt(x: x, y: y)?.usingColorSpace(.deviceRGB) else { continue }
        let value = 0.2126 * color.redComponent + 0.7152 * color.greenComponent + 0.0722 * color.blueComponent
        sum += value; peak = max(peak, value); if value < 0.04 { dark += 1 }; count += 1
    }}
    guard count > 0 else { return ImageStats(meanBrightness: 0, peakBrightness: 0, darkFraction: 1) }
    return ImageStats(meanBrightness: sum / Double(count), peakBrightness: peak, darkFraction: Double(dark) / Double(count))
}

private func warmupElapsed(_ elapsed: Double, warmup: Double) -> Bool { elapsed >= warmup }
private func frameDuration(_ fps: Double, minimum: CMTime, maximum: CMTime) -> CMTime {
    let duration = CMTimeMakeWithSeconds(1 / fps, preferredTimescale: 1_000_000_000)
    if CMTimeCompare(duration, minimum) < 0 { return minimum }
    if CMTimeCompare(duration, maximum) > 0 { return maximum }
    return duration
}
private func validCommand(_ command: String) -> Bool {
    ["list", "authorize", "inspect", "analyze", "self-test", "snapshot", "capture"].contains(command)
}
private func printJSON(_ value: Any) {
    guard let data = try? JSONSerialization.data(withJSONObject: value, options: [.prettyPrinted, .sortedKeys]),
          let string = String(data: data, encoding: .utf8) else { fail("could not encode JSON") }
    print(string)
}
private func analyze(_ path: String, requireNonblack: Bool) -> Never {
    guard let rep = NSBitmapImageRep(data: (try? Data(contentsOf: URL(fileURLWithPath: path))) ?? Data()) else {
        fail("cannot decode image: \(path)")
    }
    let stats = imageStats(rep)
    printJSON(["width": rep.pixelsWide, "height": rep.pixelsHigh, "meanBrightness": stats.meanBrightness,
               "peakBrightness": stats.peakBrightness, "darkFraction": stats.darkFraction,
               "essentiallyBlack": stats.essentiallyBlack])
    finish(requireNonblack && stats.essentiallyBlack ? 1 : 0)
}
private func selfTest() -> Never {
    func fixture(_ value: UInt8) -> NSBitmapImageRep {
        let rep = NSBitmapImageRep(bitmapDataPlanes: nil, pixelsWide: 16, pixelsHigh: 16,
                                   bitsPerSample: 8, samplesPerPixel: 3, hasAlpha: false,
                                   isPlanar: false, colorSpaceName: .deviceRGB, bytesPerRow: 0,
                                   bitsPerPixel: 0)!
        let pixels = rep.bitmapData!
        for y in 0..<16 { for x in 0..<16 {
            let offset = y * rep.bytesPerRow + x * rep.samplesPerPixel
            pixels[offset] = value; pixels[offset + 1] = value; pixels[offset + 2] = value
        }}
        return rep
    }
    let black = imageStats(fixture(0)), white = imageStats(fixture(255))
    guard black.essentiallyBlack && black.darkFraction == 1 && black.meanBrightness < 0.025 else { fail("self-test failed: black image stats") }
    guard !white.essentiallyBlack && white.peakBrightness > 0.9 && white.darkFraction == 0 else { fail("self-test failed: nonblack image stats") }
    guard !warmupElapsed(2.999, warmup: 3) && warmupElapsed(3, warmup: 3) && warmupElapsed(0, warmup: 0) else { fail("self-test failed: warmup boundary") }
    guard validCommand("inspect") && validCommand("capture") && !validCommand("unknown") else { fail("self-test failed: command validation") }
    let minimum = CMTime(value: 1_000_000, timescale: 60_000_240)
    let maximum = CMTime(value: 1, timescale: 10)
    let sixty = frameDuration(60, minimum: minimum, maximum: maximum)
    guard CMTimeCompare(sixty, minimum) >= 0 && CMTimeCompare(sixty, maximum) <= 0 else { fail("self-test failed: 60 fps duration rounding") }
    print("self-test: image stats, warmup boundary, command validation PASS")
    finish(0)
}

// Offline commands deliberately run before enumerating any capture device.
var args = Array(CommandLine.arguments.dropFirst())
if args.first == "--launch-status" {
    guard args.count >= 3, args[1].hasPrefix("/") else { usage() }
    launchStatusURL = URL(fileURLWithPath: args[1])
    args.removeFirst(2)
    atexit { writeLaunchStatus(launchExitCode) }
    writeLaunchStatus(nil)
}
guard let command = args.first, validCommand(command) else { usage() }
if command == "self-test" {
    guard args.count == 1 else { usage() }
    selfTest()
}
if command == "analyze" {
    let requireNonblack = args.count == 3 && args[2] == "--require-nonblack"
    guard args.count == 2 || requireNonblack else { usage() }
    analyze(args[1], requireNonblack: requireNonblack)
}

private struct Options {
    var device: String?, deviceID: String?, audioID: String?, framesDir: URL?, report: URL?
    var seconds = 10.0, warmup = 3.0, interval = 1.0, fps: Double?
    var format: Int?, pixelFormat: String?, maxMiB = 512
}
private func parseOptions(_ values: [String], from start: Int) -> Options {
    var o = Options(), i = start
    while i < values.count {
        guard i + 1 < values.count else { usage() }
        let key = values[i], value = values[i + 1]
        switch key {
        case "--device": o.device = value
        case "--device-id": o.deviceID = value
        case "--audio-device-id": o.audioID = value
        case "--frames-dir": o.framesDir = URL(fileURLWithPath: value, isDirectory: true).standardizedFileURL
        case "--report": o.report = URL(fileURLWithPath: value).standardizedFileURL
        case "--seconds": guard let n = Double(value), (1...600).contains(n) else { fail("duration must be between 1 and 600 seconds", code: 2) }; o.seconds = n
        case "--warmup": guard let n = Double(value), (0...30).contains(n) else { fail("warmup must be between 0 and 30 seconds", code: 2) }; o.warmup = n
        case "--interval": guard let n = Double(value), (0.1...60).contains(n) else { fail("interval must be between 0.1 and 60 seconds", code: 2) }; o.interval = n
        case "--format": guard let n = Int(value), n >= 0 else { fail("format index must be nonnegative", code: 2) }; o.format = n
        case "--fps": guard let n = Double(value), n.isFinite, n > 0 else { fail("fps must be positive", code: 2) }; o.fps = n
        case "--pixel-format": guard value.utf8.count == 4 else { fail("pixel format must be a FourCC", code: 2) }; o.pixelFormat = value
        case "--max-mib": guard let n = Int(value), (1...2048).contains(n) else { fail("max MiB must be between 1 and 2048", code: 2) }; o.maxMiB = n
        default: usage()
        }
        i += 2
    }
    guard o.device == nil || o.deviceID == nil else { fail("choose either --device or --device-id", code: 2) }
    guard o.fps == nil || o.format != nil else { fail("--fps requires --format", code: 2) }
    return o
}
private func videoDevices() -> [AVCaptureDevice] {
    AVCaptureDevice.DiscoverySession(deviceTypes: [.externalUnknown, .builtInWideAngleCamera], mediaType: .video, position: .unspecified).devices
}
private func authStatus(_ media: AVMediaType) -> String {
    switch AVCaptureDevice.authorizationStatus(for: media) {
    case .authorized: return "authorized"
    case .notDetermined: return "not_determined"
    case .denied: return "denied"
    case .restricted: return "restricted"
    @unknown default: return "unknown"
    }
}
@Sendable private func fourCC(_ code: FourCharCode) -> String {
    if code == kCVPixelFormatType_32ARGB { return "ARGB" }
    return String(bytes: [UInt8((code >> 24) & 255), UInt8((code >> 16) & 255), UInt8((code >> 8) & 255), UInt8(code & 255)], encoding: .macOSRoman) ?? String(format: "0x%08x", code)
}
private func formatInfo(_ f: AVCaptureDevice.Format, _ i: Int) -> [String: Any] {
    let d = CMVideoFormatDescriptionGetDimensions(f.formatDescription)
    return ["index": i, "width": Int(d.width), "height": Int(d.height), "fourCC": fourCC(CMFormatDescriptionGetMediaSubType(f.formatDescription)),
            "fpsRanges": f.videoSupportedFrameRateRanges.map { ["min": $0.minFrameRate, "max": $0.maxFrameRate] }]
}
private func deviceInfo(_ d: AVCaptureDevice) -> [String: Any] {
    ["id": d.uniqueID, "name": d.localizedName, "busy": d.isInUseByAnotherApplication,
     "formats": d.formats.enumerated().map { formatInfo($0.element, $0.offset) },
     "controlsSupported": ["focus": d.isFocusModeSupported(.autoFocus) || d.isFocusModeSupported(.continuousAutoFocus), "whiteBalance": d.isWhiteBalanceModeSupported(.autoWhiteBalance) || d.isWhiteBalanceModeSupported(.continuousAutoWhiteBalance), "exposure": d.isExposureModeSupported(.autoExpose) || d.isExposureModeSupported(.continuousAutoExposure)],
     "controlsNotExposed": ["zoom", "exposureBias", "vendorExtensionUnits"],
     "controls": ["focusMode": d.focusMode.rawValue, "whiteBalanceMode": d.whiteBalanceMode.rawValue, "exposureMode": d.exposureMode.rawValue]]
}
private func resolveDevice(_ ds: [AVCaptureDevice], _ o: Options) -> AVCaptureDevice? {
    if let id = o.deviceID { return ds.first { $0.uniqueID == id } }
    let matches = ds.filter { $0.localizedName == (o.device ?? defaultDevice) }
    if matches.count > 1 { fail("duplicate device name; use --device-id") }
    return matches.first
}
private final class CaptureState {
    private let lock = NSLock()
    private var running = false, recording = false, finished = false, frames = 0, drops = 0, jpegCount = 0
    private var startedAt: Double?, stoppedAt: Double?, errorMessage: String?, signal = ""
    private var samples = [[String: Any]]()
    private var assetResult: (Double, Int, [[String: Any]])?, assetError: String?
    func assetComplete(_ result: (Double, Int, [[String: Any]])?, _ error: String?) { lock.lock(); assetResult = result; assetError = error; lock.unlock() }
    func assetInfo() -> ((Double, Int, [[String: Any]])?, String?) { lock.lock(); defer { lock.unlock() }; return (assetResult, assetError) }
    func session(_ value: Bool) { lock.lock(); running = value; if !value { errorMessage = "capture session failed to start" }; lock.unlock() }
    func sessionStopped() { lock.lock(); running = false; lock.unlock() }
    func movieStarted() { lock.lock(); recording = true; startedAt = ProcessInfo.processInfo.systemUptime; lock.unlock() }
    func movieStopped() { lock.lock(); if stoppedAt == nil { stoppedAt = ProcessInfo.processInfo.systemUptime }; lock.unlock() }
    func finish(_ error: String?) { lock.lock(); finished = true; if let error { errorMessage = error }; lock.unlock() }
    func failed(_ message: String) { lock.lock(); if errorMessage == nil { errorMessage = message }; lock.unlock() }
    func setSignal(_ name: String) { lock.lock(); signal = name; lock.unlock() }
    func isRunning() -> Bool { lock.lock(); defer { lock.unlock() }; return running }
    func isRecording() -> Bool { lock.lock(); defer { lock.unlock() }; return recording }
    func isFinished() -> Bool { lock.lock(); defer { lock.unlock() }; return finished }
    func started() -> Double? { lock.lock(); defer { lock.unlock() }; return startedAt }
    func stopped() -> Double? { lock.lock(); defer { lock.unlock() }; return stoppedAt }
    func getFailure() -> String? { lock.lock(); defer { lock.unlock() }; return errorMessage }
    func drop() { lock.lock(); drops += 1; lock.unlock() }
    func frame(_ sample: [String: Any]) -> Int { lock.lock(); defer { lock.unlock() }; frames += 1; samples.append(sample); return samples.count - 1 }
    func updateFrame(_ index: Int, _ values: [String: Any]) { lock.lock(); if samples.indices.contains(index) { values.forEach { samples[index][$0] = $1 } }; lock.unlock() }
    func nextJPEG() -> Int { lock.lock(); defer { lock.unlock() }; defer { jpegCount += 1 }; return jpegCount }
    func values() -> (Int, Int, Int, [[String: Any]], String) { lock.lock(); defer { lock.unlock() }; return (frames, drops, jpegCount, samples, signal) }
}
private final class CaptureDelegate: NSObject, AVCaptureVideoDataOutputSampleBufferDelegate, AVCaptureFileOutputRecordingDelegate {
    let state: CaptureState, epoch: Double, options: Options, snapshotURL: URL?, ci = CIContext()
    private var lastSample = -Double.infinity, snapshotDone = false
    private var firstFrameAt: Double?
    init(_ state: CaptureState, _ epoch: Double, _ options: Options, _ snapshotURL: URL?) { self.state = state; self.epoch = epoch; self.options = options; self.snapshotURL = snapshotURL }
    func captureOutput(_ output: AVCaptureOutput, didDrop sampleBuffer: CMSampleBuffer, from connection: AVCaptureConnection) { state.drop() }
    func captureOutput(_ output: AVCaptureOutput, didOutput sampleBuffer: CMSampleBuffer, from connection: AVCaptureConnection) {
        autoreleasepool {
            let elapsed = ProcessInfo.processInfo.systemUptime - epoch, pts = CMSampleBufferGetPresentationTimeStamp(sampleBuffer)
            if firstFrameAt == nil { firstFrameAt = elapsed }
            guard let pb = CMSampleBufferGetImageBuffer(sampleBuffer) else { state.failed("frame has no image buffer"); return }
            let p = CMTimeGetSeconds(pts), index = state.frame(["monotonicSeconds": elapsed, "ptsSeconds": pts.isValid && p.isFinite ? p : NSNull(), "width": CVPixelBufferGetWidth(pb), "height": CVPixelBufferGetHeight(pb), "pixelFormat": fourCC(CVPixelBufferGetPixelFormatType(pb))])
            let settled = warmupElapsed(elapsed - firstFrameAt!, warmup: options.warmup)
            let snapshotFrame = settled && snapshotURL != nil && !snapshotDone
            let sampleFrame = settled && elapsed - lastSample >= options.interval
            guard snapshotFrame || sampleFrame else { return }
            guard let cg = ci.createCGImage(CIImage(cvPixelBuffer: pb), from: CGRect(x: 0, y: 0, width: CVPixelBufferGetWidth(pb), height: CVPixelBufferGetHeight(pb))) else { state.failed("cannot decode sampled frame"); return }
            let rep = NSBitmapImageRep(cgImage: cg)
            let stats = imageStats(rep)
            state.updateFrame(index, ["meanBrightness": stats.meanBrightness, "peakBrightness": stats.peakBrightness, "darkFraction": stats.darkFraction, "essentiallyBlack": stats.essentiallyBlack])
            lastSample = elapsed
            if snapshotFrame, let snapshotURL { snapshotDone = writeJPEG(rep, snapshotURL); if !snapshotDone { state.failed("snapshot JPEG write failed") } }
            if sampleFrame, let dir = options.framesDir {
                let url = dir.appendingPathComponent(String(format: "frame-%06d.jpg", state.nextJPEG()))
                if writeJPEG(rep, url) { state.updateFrame(index, ["jpeg": url.lastPathComponent]) }
                else { state.failed("periodic JPEG write failed: \(url.path)") }
            }
        }
    }
    func fileOutput(_ output: AVCaptureFileOutput, didStartRecordingTo fileURL: URL, from connections: [AVCaptureConnection]) { state.movieStarted() }
    func fileOutput(_ output: AVCaptureFileOutput, didFinishRecordingTo outputFileURL: URL, from connections: [AVCaptureConnection], error: Error?) {
        state.movieStopped()
        if (error as NSError?)?.code == AVError.Code.maximumFileSizeReached.rawValue { state.failed("movie reached its file-size cap before the capture window ended") }
        let successful = (error as NSError?)?.userInfo[AVErrorRecordingSuccessfullyFinishedKey] as? Bool ?? false
        state.finish(successful ? nil : error?.localizedDescription)
    }
    private func writeJPEG(_ rep: NSBitmapImageRep, _ url: URL) -> Bool { guard let data = rep.representation(using: .jpeg, properties: [.compressionFactor: 0.88]) else { return false }; return (try? data.write(to: url, options: .atomic)) != nil }
}

// Main-run-loop capture and bounded recording pattern: WiiXplorer-NG
// e85f62851547c0e7dbf74897234eafc1c3621bac, scripts/wii-capture.m.
private func pump(_ seconds: Double = 0.05) {
    RunLoop.current.run(until: Date(timeIntervalSinceNow: seconds))
}
private func authorize(_ media: AVMediaType) {
    if authStatus(media) == "not_determined" {
        var decision: Bool?
        AVCaptureDevice.requestAccess(for: media) { value in
            DispatchQueue.main.async { decision = value }
        }
        let deadline = Date(timeIntervalSinceNow: 180)
        while decision == nil && Date() < deadline { pump() }
        guard decision != nil else { fail("permission decision timed out") }
    }
    guard authStatus(media) == "authorized" else {
        fail("\(media == .audio ? "Microphone/HDMI audio" : "Camera") permission is \(authStatus(media)); enable access for Wii64 HDMI Capture in macOS Privacy settings")
    }
    print("\(media.rawValue) access authorized.")
}
private func requireNew(_ url: URL) {
    let fm = FileManager.default
    guard !fm.fileExists(atPath: url.path) else { fail("output already exists: \(url.path)") }
    var directory: ObjCBool = false
    guard fm.fileExists(atPath: url.deletingLastPathComponent().path, isDirectory: &directory), directory.boolValue else {
        fail("output directory does not exist: \(url.deletingLastPathComponent().path)")
    }
}
let devices = videoDevices()
let audioDevices = AVCaptureDevice.DiscoverySession(deviceTypes: [.builtInMicrophone, .externalUnknown], mediaType: .audio, position: .unspecified).devices
if command == "list" {
    guard args.count == 1 else { usage() }
    printJSON(["permissionHost": Bundle.main.bundleIdentifier ?? "unbundled",
               "cameraAuthorization": authStatus(.video), "audioAuthorization": authStatus(.audio),
               "video": devices.map { ["name": $0.localizedName, "id": $0.uniqueID] },
               "audio": audioDevices.map { ["name": $0.localizedName, "id": $0.uniqueID] }])
    finish(0)
}
if command == "authorize" {
    guard args.count == 1 || args == ["authorize", "--audio"] else { usage() }
    authorize(args.count == 1 ? .video : .audio)
    finish(0)
}
private let options = parseOptions(args, from: command == "inspect" ? 1 : 2)
if command != "inspect" {
    guard args.count >= 2 else { usage() }
    let prospective = URL(fileURLWithPath: args[1]).standardizedFileURL
    requireNew(prospective)
    if let report = options.report {
        guard report != prospective else { fail("report and media output must differ", code: 2) }
        requireNew(report)
    }
    if let dir = options.framesDir { requireNew(dir) }
} else if stride(from: 1, to: args.count, by: 2).contains(where: { !["--device", "--device-id"].contains(args[$0]) }) {
    usage()
}
guard let device = resolveDevice(devices, options) else { fail("selected device not found; run 'wii_video list'") }
if command == "inspect" {
    var info = deviceInfo(device)
    if authStatus(.video) == "authorized", !device.isInUseByAnotherApplication {
        do {
            let session = AVCaptureSession(), video = AVCaptureVideoDataOutput()
            let input = try AVCaptureDeviceInput(device: device)
            if session.canAddInput(input) {
                session.addInput(input)
                if session.canAddOutput(video) { session.addOutput(video) }
                info["decodedPixelFormats"] = video.availableVideoPixelFormatTypes.map { fourCC($0) }
            }
        } catch { info["inspectionError"] = error.localizedDescription }
    }
    info["cameraAuthorization"] = authStatus(.video)
    info["audioAuthorization"] = authStatus(.audio)
    info["audio"] = audioDevices.map { ["id": $0.uniqueID, "name": $0.localizedName] }
    printJSON(info)
    finish(0)
}
guard args.count >= 2 else { usage() }
let outputURL = URL(fileURLWithPath: args[1]).standardizedFileURL
if let index = options.format, index >= device.formats.count { fail("unsupported format index", code: 2) }
guard authStatus(.video) == "authorized" else { fail("camera access is \(authStatus(.video)); run 'wii_video authorize' explicitly") }
guard ProcessInfo.processInfo.environment["WII_BENCH_JOB_START"] != nil,
      ProcessInfo.processInfo.environment["WII_BENCH_IP"] != nil else {
    fail("capture requires an active Wii queue job; use scripts/wii_video_capture.sh")
}
guard !device.isInUseByAnotherApplication else { fail("capture device is busy in another application") }

let session = AVCaptureSession(), video = AVCaptureVideoDataOutput(), movie = AVCaptureMovieFileOutput()
private let state = CaptureState(), serial = DispatchQueue(label: "wii_video.frames")
let epoch = ProcessInfo.processInfo.systemUptime
private let delegate = CaptureDelegate(state, epoch, options, command == "snapshot" ? outputURL : nil)
var observers = [NSObjectProtocol]()
var signals = [DispatchSourceSignal]()
do {
    session.beginConfiguration()
    let input = try AVCaptureDeviceInput(device: device)
    guard session.canAddInput(input) else { fail("cannot add selected video input") }
    session.addInput(input)
    if let index = options.format {
        try device.lockForConfiguration()
        defer { device.unlockForConfiguration() }
        let format = device.formats[index]
        if let fps = options.fps {
            guard format.videoSupportedFrameRateRanges.contains(where: { fps >= $0.minFrameRate && fps <= $0.maxFrameRate }) else {
                fail("fps unsupported for selected format", code: 2)
            }
        }
        device.activeFormat = format
    }
    if let audioID = options.audioID {
        guard command == "capture" else { fail("audio requires movie capture", code: 2) }
        guard authStatus(.audio) == "authorized" else { fail("audio access is \(authStatus(.audio)); run 'wii_video authorize --audio' explicitly") }
        guard let audio = audioDevices.first(where: { $0.uniqueID == audioID }) else { fail("audio device ID not found") }
        let audioInput = try AVCaptureDeviceInput(device: audio)
        guard session.canAddInput(audioInput) else { fail("cannot add selected audio input") }
        session.addInput(audioInput)
    }
    video.alwaysDiscardsLateVideoFrames = true
    video.setSampleBufferDelegate(delegate, queue: serial)
    guard session.canAddOutput(video) else { fail("cannot add video sample output") }
    session.addOutput(video)
    if let pixel = options.pixelFormat {
        guard let code = video.availableVideoPixelFormatTypes.first(where: { fourCC($0) == pixel }) else {
            fail("unsupported decoded pixel format; available: \(video.availableVideoPixelFormatTypes.map { fourCC($0) })", code: 2)
        }
        video.videoSettings = [kCVPixelBufferPixelFormatTypeKey as String: code]
    }
    if command == "capture" {
        guard session.canAddOutput(movie) else { fail("cannot add movie output") }
        session.addOutput(movie)
        movie.maxRecordedDuration = CMTime(seconds: options.seconds + options.warmup, preferredTimescale: 1_000)
        movie.maxRecordedFileSize = Int64(options.maxMiB) * 1024 * 1024
    }
    session.commitConfiguration()
    // Encoder settings can renegotiate the session. Set them before startup,
    // then apply the requested input format/rate after that negotiation.
    if command == "capture", let connection = movie.connection(with: .video) {
        let format = options.format.map { device.formats[$0] } ?? device.activeFormat
        let dimension = CMVideoFormatDescriptionGetDimensions(format.formatDescription)
        movie.setOutputSettings([AVVideoCodecKey: AVVideoCodecType.h264,
                                 AVVideoWidthKey: Int(dimension.width), AVVideoHeightKey: Int(dimension.height)], for: connection)
    }
    if let dir = options.framesDir { try FileManager.default.createDirectory(at: dir, withIntermediateDirectories: false) }
    for event in [NSNotification.Name.AVCaptureSessionRuntimeError, NSNotification.Name.AVCaptureSessionWasInterrupted] {
        observers.append(NotificationCenter.default.addObserver(forName: event, object: session, queue: nil) { note in
            let error = note.userInfo?[AVCaptureSessionErrorKey] as? NSError
            state.failed(error?.localizedDescription ?? "capture session interrupted")
        })
    }
    for number in [SIGINT, SIGTERM] {
        signal(number, SIG_IGN)
        let source = DispatchSource.makeSignalSource(signal: number, queue: .main)
        source.setEventHandler { state.setSignal(number == SIGINT ? "SIGINT" : "SIGTERM") }
        source.resume()
        signals.append(source)
    }
    let control = DispatchQueue(label: "wii_video.session")
    control.async { session.startRunning(); state.session(session.isRunning) }
    let startDeadline = Date(timeIntervalSinceNow: 15)
    while !state.isRunning() && state.getFailure() == nil && Date() < startDeadline { pump() }
    if !state.isRunning() { state.failed("capture session did not start before deadline") }
    // Startup reset both resolution and timing on this Mac. Apply the advertised
    // mode after it starts, then verify post-warmup frames rather than trust it.
    if state.isRunning(), let index = options.format {
        try device.lockForConfiguration()
        device.activeFormat = device.formats[index]
        if let fps = options.fps {
            let range = device.formats[index].videoSupportedFrameRateRanges.first { fps >= $0.minFrameRate && fps <= $0.maxFrameRate }!
            let duration = frameDuration(fps, minimum: range.minFrameDuration, maximum: range.maxFrameDuration)
            device.activeVideoMaxFrameDuration = duration
            device.activeVideoMinFrameDuration = duration
        }
        device.unlockForConfiguration()
    }
    // Include preroll in the movie so encoder startup does not consume the
    // requested post-warmup capture window. JPEGs still start after warmup.
    if command == "capture", state.getFailure() == nil, state.values().4.isEmpty {
        movie.startRecording(to: outputURL, recordingDelegate: delegate)
    }
    let settle = ProcessInfo.processInfo.systemUptime + options.warmup
    while state.getFailure() == nil && ProcessInfo.processInfo.systemUptime < settle && state.values().4.isEmpty { pump() }
    let deadline = ProcessInfo.processInfo.systemUptime + (command == "snapshot" ? 15 : options.seconds + 15)
    while state.getFailure() == nil && state.values().4.isEmpty && ProcessInfo.processInfo.systemUptime < deadline {
        if command == "snapshot" && FileManager.default.fileExists(atPath: outputURL.path) { break }
        if command == "capture" && state.isFinished() { break }
        // The encoder can start after its didStart callback. Stop at media
        // duration, not host time, or a nominal 12-second clip can be 11 seconds.
        pump()
    }
    if movie.isRecording { state.movieStopped(); movie.stopRecording() }
    let finishDeadline = Date(timeIntervalSinceNow: 10)
    while command == "capture" && state.isRecording() && !state.isFinished() && Date() < finishDeadline { pump() }
    if command == "capture" && !state.isFinished() { state.failed("movie failed to finalize") }
    if command == "snapshot" && !FileManager.default.fileExists(atPath: outputURL.path) { state.failed("no snapshot before deadline") }
    control.async { session.stopRunning(); state.sessionStopped() }
    let stopDeadline = Date(timeIntervalSinceNow: 5)
    while state.isRunning() && Date() < stopDeadline { pump() }
    if state.isRunning() { state.failed("capture session did not stop before deadline") }
    // No file output is still recording. Detach and drain the serial frame writer.
    video.setSampleBufferDelegate(nil, queue: nil)
    serial.sync {}
    for observer in observers { NotificationCenter.default.removeObserver(observer) }
    for source in signals { source.cancel() }
    let values = state.values()
    let settledSamples = values.3.filter { ($0["monotonicSeconds"] as? Double ?? 0) >= (values.3.first?["monotonicSeconds"] as? Double ?? 0) + options.warmup }
    if let index = options.format {
        let dimension = CMVideoFormatDescriptionGetDimensions(device.formats[index].formatDescription)
        if settledSamples.contains(where: { ($0["width"] as? Int) != Int(dimension.width) || ($0["height"] as? Int) != Int(dimension.height) }) {
            state.failed("delivered dimensions do not match requested format; inspect retained report")
        }
    }
    let pts = values.3.compactMap { $0["ptsSeconds"] as? Double }
    let span = (pts.last ?? 0) - (pts.first ?? 0)
    let elapsed = ProcessInfo.processInfo.systemUptime - epoch
    var report: [String: Any] = ["device": device.uniqueID, "name": device.localizedName,
        "activeFormat": formatInfo(device.activeFormat, device.formats.firstIndex(of: device.activeFormat) ?? -1),
        "frames": values.0, "drops": values.1, "jpegCount": values.2, "samples": values.3,
        "observedFPS": span > 0 ? Double(max(0, pts.count - 1)) / span : 0,
        "elapsedSeconds": elapsed, "requestedSeconds": options.seconds, "warmupSeconds": options.warmup,
        "movieStartedSeconds": state.started().map { $0 - epoch } as Any? ?? NSNull(),
        "movieStopSeconds": state.stopped().map { $0 - epoch } as Any? ?? NSNull(),
        "requestedFPS": options.fps ?? 0,
        "activeMinFrameDurationSeconds": CMTimeGetSeconds(device.activeVideoMinFrameDuration),
        "activeMaxFrameDurationSeconds": CMTimeGetSeconds(device.activeVideoMaxFrameDuration),
        "interruptedBy": values.4, "movieFinalized": state.isFinished(),
        "audioDeviceID": options.audioID ?? "", "output": outputURL.path,
        "queueJob": ProcessInfo.processInfo.environment["WII_BENCH_JOB"] ?? ""]
    if let index = options.format { report["requestedFormat"] = formatInfo(device.formats[index], index) }
    if command == "capture" && state.isFinished() {
        let asset = AVURLAsset(url: outputURL)
        let task = Task {
            do {
                let duration = try await asset.load(.duration)
                let tracks = try await asset.loadTracks(withMediaType: .audio)
                let videoTracks = try await asset.loadTracks(withMediaType: .video)
                var videoFormats = [[String: Any]]()
                for track in videoTracks {
                    let rate = try await track.load(.nominalFrameRate)
                    for description in try await track.load(.formatDescriptions) {
                        let dimension = CMVideoFormatDescriptionGetDimensions(description)
                        videoFormats.append(["width": Int(dimension.width), "height": Int(dimension.height), "codec": fourCC(CMFormatDescriptionGetMediaSubType(description)), "nominalFPS": rate.isFinite ? Double(rate) : 0])
                    }
                }
                state.assetComplete((CMTimeGetSeconds(duration), tracks.count, videoFormats), nil)
            } catch {
                state.assetComplete(nil, error.localizedDescription)
            }
        }
        let assetDeadline = Date(timeIntervalSinceNow: 5)
        while state.assetInfo().0 == nil && state.assetInfo().1 == nil && Date() < assetDeadline { pump() }
        task.cancel()
        let (assetResult, assetError) = state.assetInfo()
        if let error = assetError { state.failed("movie inspection: \(error)") }
        if assetResult == nil { state.failed("movie inspection did not finish") }
        let duration = assetResult?.0 ?? 0
        report["movieDurationSeconds"] = duration.isFinite ? duration : 0
        report["audioTracks"] = assetResult?.1 ?? 0
        report["videoTracks"] = assetResult?.2 ?? []
        if assetResult?.2.isEmpty != false { state.failed("movie has no encoded video track") }
        report["movieFinalized"] = state.isFinished() && assetResult?.2.isEmpty == false && duration.isFinite && duration > 0
        if let index = options.format {
            let dimension = CMVideoFormatDescriptionGetDimensions(device.formats[index].formatDescription)
            if assetResult?.2.contains(where: { ($0["width"] as? Int) != Int(dimension.width) || ($0["height"] as? Int) != Int(dimension.height) }) == true {
                state.failed("encoded dimensions do not match requested format; inspect retained report")
            }
        }
        report["fileBytes"] = (try? FileManager.default.attributesOfItem(atPath: outputURL.path)[.size]) ?? 0
        if (!duration.isFinite || duration + 0.25 < options.seconds) && values.4.isEmpty {
            state.failed("movie ended before requested duration (check size cap/session errors)")
        }
    }
    if values.0 == 0 { state.failed("capture delivered no frames") }
    report["error"] = state.getFailure() ?? ""
    if let path = options.report {
        try JSONSerialization.data(withJSONObject: report, options: [.prettyPrinted, .sortedKeys]).write(to: path, options: .atomic)
    }
    if let error = state.getFailure() { fail(error) }
    if !values.4.isEmpty { fail("capture interrupted by \(values.4); finalized output retained", code: 130) }
    print("Saved \(outputURL.path); \(values.0) frames, \(values.1) drops, \(report["observedFPS"]!) delivered FPS")
} catch {
    fail("capture setup failed: \(error.localizedDescription)")
}
finish(0)
