import AVFoundation
import AppKit
import CoreImage
import Foundation

private let defaultDevice = "UGREEN 15389"

private func fail(_ message: String, code: Int32 = 1) -> Never {
    fputs("wii_video: \(message)\n", stderr)
    exit(code)
}

private func usage() -> Never {
    fail("usage: wii_video {list|authorize|snapshot <output.jpg>|capture <output.mov>} [--device NAME] [--seconds 1...300]", code: 2)
}

private final class RecordingDelegate: NSObject, AVCaptureFileOutputRecordingDelegate {
    let done = DispatchSemaphore(value: 0)
    var error: Error?

    func fileOutput(_ output: AVCaptureFileOutput, didFinishRecordingTo outputFileURL: URL,
                    from connections: [AVCaptureConnection], error: Error?) {
        self.error = error
        done.signal()
    }
}

private final class SnapshotDelegate: NSObject, AVCaptureVideoDataOutputSampleBufferDelegate {
    let done = DispatchSemaphore(value: 0)
    private let context = CIContext()
    private let outputURL: URL
    private var finished = false
    var error: Error?

    init(outputURL: URL) { self.outputURL = outputURL }

    func captureOutput(_ output: AVCaptureOutput, didOutput sampleBuffer: CMSampleBuffer,
                       from connection: AVCaptureConnection) {
        guard !finished else { return }
        finished = true
        do {
            guard let buffer = CMSampleBufferGetImageBuffer(sampleBuffer) else {
                throw NSError(domain: "wii_video", code: 1,
                              userInfo: [NSLocalizedDescriptionKey: "camera returned no image"])
            }
            let source = CIImage(cvPixelBuffer: buffer)
            guard let image = context.createCGImage(source, from: source.extent) else {
                throw NSError(domain: "wii_video", code: 1,
                              userInfo: [NSLocalizedDescriptionKey: "could not decode camera image"])
            }
            let bitmap = NSBitmapImageRep(cgImage: image)
            guard let jpeg = bitmap.representation(using: .jpeg,
                                                   properties: [.compressionFactor: 0.9]) else {
                throw NSError(domain: "wii_video", code: 2,
                              userInfo: [NSLocalizedDescriptionKey: "could not encode JPEG"])
            }
            try jpeg.write(to: outputURL, options: .atomic)
        } catch {
            self.error = error
        }
        done.signal()
    }
}

private func authorizeCamera() {
    switch AVCaptureDevice.authorizationStatus(for: .video) {
    case .authorized:
        print("Camera access is already authorized.")
    case .notDetermined:
        let done = DispatchSemaphore(value: 0)
        var granted = false
        AVCaptureDevice.requestAccess(for: .video) { value in
            granted = value
            done.signal()
        }
        guard done.wait(timeout: .now() + 60) == .success else {
            fail("timed out waiting for the camera permission decision")
        }
        guard granted else {
            fail("camera access was denied; enable it for Terminal in System Settings > Privacy & Security > Camera")
        }
        print("Camera access authorized.")
    case .denied:
        fail("camera access is denied; enable it for Terminal in System Settings > Privacy & Security > Camera")
    case .restricted:
        fail("camera access is restricted by macOS or device policy")
    @unknown default:
        fail("macOS returned an unknown camera authorization status")
    }
}

let args = Array(CommandLine.arguments.dropFirst())
guard let command = args.first else { usage() }
let devices = AVCaptureDevice.DiscoverySession(
    deviceTypes: [.externalUnknown, .builtInWideAngleCamera],
    mediaType: .video,
    position: .unspecified
).devices

if command == "list" {
    guard args.count == 1 else { usage() }
    if devices.isEmpty {
        print("No video capture devices found.")
    } else {
        for device in devices { print(device.localizedName) }
    }
    exit(0)
}

if command == "authorize" {
    guard args.count == 1 else { usage() }
    authorizeCamera()
    exit(0)
}

guard command == "snapshot" || command == "capture", args.count >= 2 else { usage() }
let outputURL = URL(fileURLWithPath: args[1]).standardizedFileURL
var deviceName = defaultDevice
var seconds = 10
var index = 2
while index < args.count {
    guard index + 1 < args.count else { usage() }
    switch args[index] {
    case "--device": deviceName = args[index + 1]
    case "--seconds":
        guard let parsed = Int(args[index + 1]) else { usage() }
        seconds = parsed
    default: usage()
    }
    index += 2
}
guard command == "snapshot" || (1...300).contains(seconds) else {
    fail("duration must be between 1 and 300 seconds", code: 2)
}
guard !FileManager.default.fileExists(atPath: outputURL.path) else {
    fail("output already exists: \(outputURL.path)")
}
guard FileManager.default.fileExists(atPath: outputURL.deletingLastPathComponent().path) else {
    fail("output directory does not exist: \(outputURL.deletingLastPathComponent().path)")
}
switch AVCaptureDevice.authorizationStatus(for: .video) {
case .authorized: break
case .notDetermined:
    fail("camera access has not been authorized; run 'wii_video authorize' in Terminal for the macOS permission prompt")
case .denied:
    fail("camera access is denied; enable it for Terminal in System Settings > Privacy & Security > Camera")
case .restricted:
    fail("camera access is restricted by macOS or device policy")
@unknown default:
    fail("macOS returned an unknown camera authorization status")
}
guard let device = devices.first(where: { $0.localizedName == deviceName }) else {
    fail("device '\(deviceName)' not found; run 'wii_video list' to inspect video devices")
}

do {
    let input = try AVCaptureDeviceInput(device: device)
    let session = AVCaptureSession()
    guard session.canAddInput(input) else { fail("AVFoundation cannot configure '\(deviceName)'") }
    session.addInput(input)

    if command == "snapshot" {
        let video = AVCaptureVideoDataOutput()
        video.alwaysDiscardsLateVideoFrames = true
        let delegate = SnapshotDelegate(outputURL: outputURL)
        video.setSampleBufferDelegate(delegate, queue: DispatchQueue(label: "wii_video.frame"))
        guard session.canAddOutput(video) else { fail("AVFoundation cannot produce a video frame") }
        session.addOutput(video)
        session.startRunning()
        guard delegate.done.wait(timeout: .now() + 15) == .success else {
            session.stopRunning()
            fail("timed out waiting for a frame from '\(deviceName)'")
        }
        session.stopRunning()
        if let error = delegate.error { fail("snapshot failed: \(error.localizedDescription)") }
    } else {
        let movie = AVCaptureMovieFileOutput()
        guard session.canAddOutput(movie) else { fail("AVFoundation cannot record from '\(deviceName)'") }
        session.addOutput(movie)
        session.sessionPreset = .high
        let delegate = RecordingDelegate()
        session.startRunning()
        movie.startRecording(to: outputURL, recordingDelegate: delegate)
        DispatchQueue.global().asyncAfter(deadline: .now() + .seconds(seconds)) {
            movie.stopRecording()
        }
        guard delegate.done.wait(timeout: .now() + .seconds(seconds + 30)) == .success else {
            session.stopRunning()
            fail("timed out waiting for the recording to finish")
        }
        session.stopRunning()
        if let error = delegate.error { fail("capture failed: \(error.localizedDescription)") }
    }
    print("Saved \(outputURL.path)")
} catch {
    fail("capture setup failed: \(error.localizedDescription)")
}
