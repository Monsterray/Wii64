# Wii64 audio

The emulator reproduces each game's N64 audio processing and sends the result to the host sound system.

## Language

**N64 voice**:
One sound source scheduled by a game's audio driver or microcode before the N64 stereo mix. Its identity and count depend on the game.
_Avoid_: N64 hardware channel

**N64 mix**:
The stereo PCM result of the game's synthesis, gain, pan, and effects. It does not retain separate voice identities.
_Avoid_: N64 voice

**Wii AESND voice**:
One independently controlled playback stream in the Wii DSP mixer. It is not the same thing as an N64 voice.
_Avoid_: N64 channel

**Audio output channel**:
The left or right channel of the final stereo signal, not an instrument or sound source.
_Avoid_: voice
