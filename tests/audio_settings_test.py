#!/usr/bin/env python3
"""Compile the actual config routines and Audio toggle callbacks with tiny UI stubs."""
import os
import pathlib
import subprocess
import tempfile

root = pathlib.Path(__file__).resolve().parents[1]
source = (root / "main/main_gc-menu2.cpp").read_text()
options = "\n".join(line for line in source.splitlines() if '{ "Audio' in line)
assert options.count('{ "Audio') == 6
routines = source[source.index("void setOption(char* key, int value){"):]
settings = (root / "menu/SettingsFrame.cpp").read_text()
callbacks = settings[settings.index("void Func_AudioOn()\n"):
                     settings.index("void Func_AutoSaveNativeYes()\n")]
advanced = (root / "menu/AdvancedAudioFrame.cpp").read_text()
controls = advanced[advanced.index("static char *settings[5]"):advanced.index("extern MenuContext")]
program = r'''
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "main/wii64config.h"
char audioEnabled, audioQuality, audioOutputResampler, audioMixerPrecision, audioLatency, audioSync;
static struct { const char *key; char *value; char min, max; } OPTIONS[] = {
''' + options + r'''
};
struct Button { bool selected; void setSelected(bool value) { selected = value; } } buttons[42];
struct { Button *button; } FRAME_BUTTONS[42];
''' + routines + callbacks + "\nstatic char values[5][32];\n" + controls + r'''
static void pair(const char *text) {
    char buffer[256]; snprintf(buffer, sizeof(buffer), "%s", text); handleConfigPair(buffer);
}
int main() {
    for (int i=0; i<42; i++) FRAME_BUTTONS[i].button = &buttons[i];
    Func_AudioOn(); assert(audioEnabled == AUDIO_ENABLE && buttons[35].selected && !buttons[36].selected);
    Func_AudioOff(); assert(audioEnabled == AUDIO_DISABLE && !buttons[35].selected && buttons[36].selected);
    void (*actions[5])() = { synthesis, output, mixer, latency, sync };
    for (int row=0; row<5; row++) {
        *settings[row]=0; refresh();
        for (int value=1; value<=counts[row]; value++) {
            actions[row]();
            assert(*settings[row]==value % counts[row]);
            assert(!strcmp(values[row], names[row][value % counts[row]]));
        }
    }
    pair("AudioQuality=2"); pair("AudioOutputResampler = 1"); pair("AudioMixerPrecision:1");
    pair("  AudioLatency = 0"); pair("AudioSync=2"); pair("Audio=1");
    assert(audioQuality==2 && audioOutputResampler==1 && audioMixerPrecision==1 && audioLatency==0 && audioSync==2);
    const char *bad[] = { "", "# comment", "nodelimiter", "AudioSync=258", "AudioSync=-1",
        "AudioSync=oops", "AudioSync=9999999999999999999999999999999", "AudioQuality=3" };
    for (const char *value : bad) pair(value);
    assert(audioQuality==2 && audioSync==2);
    FILE *file=tmpfile(); assert(file); writeConfig(file); rewind(file);
    audioQuality=audioOutputResampler=audioMixerPrecision=audioLatency=audioSync=audioEnabled=0;
    readConfig(file); fclose(file);
    assert(audioEnabled==1 && audioQuality==2 && audioOutputResampler==1 && audioMixerPrecision==1 && audioSync==2);
    // Existing AudioQuality numbers retain their meaning; missing keys stay unchanged.
    audioLatency=AUDIOLATENCY_STABLE; pair("AudioQuality = 1");
    assert(audioQuality==AUDIOQUALITY_FAST && audioLatency==AUDIOLATENCY_STABLE);
    puts("audio config round-trip, bounds, old keys, On/Off and five row callbacks: ok");
}
'''
with tempfile.TemporaryDirectory(prefix="wii64-settings-") as directory:
    path = pathlib.Path(directory)
    (path / "test.cpp").write_text(program)
    subprocess.run([os.environ.get("CXX", "c++"), "-std=c++11", "-Wall", "-Wextra", "-Werror",
                    "-fsanitize=address,undefined", "-fno-sanitize-recover=all", "-I", str(root),
                    str(path / "test.cpp"), "-o", str(path / "test")], check=True)
    subprocess.run([str(path / "test")], check=True)
