# 2026-10-01_wii_audio_full2

- date: 2026-10-01
- platform: hardware
- plugin: glN64
- build: 76a86b6+system-probes-1.6.5
- purpose: Six-ROM audio-system title/replay survey, original full probes. Loads, replays and HBC return validated; first full run had a host launcher error, so not a fully validated probe-cost triple.
- screenshots: baselines/media/2026-10-01_wii_audio_full2/ (not in git)

| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | GoldenEye 007 (USA).z64 | vis | 900 | 15.18 | 0.988 | 68.7 | 52.4 | 5541 | 837 | 15 | 3 |
| 2 | Diddy Kong Racing (USA) (En,Fr) (Rev 1).z64 | vis | 900 | 15.16 | 0.989 | 71.4 | 27.2 | 6302 | 946 | 13 | 0 |
| 3 | Wave Race 64 - Kawasaki Jet Ski (USA) (Rev 1).z64 | vis | 900 | 15.27 | 0.983 | 44.6 | 17.1 | 6648 | 866 | 12 | 9 |
| 4 | 007 - The World Is Not Enough (USA).z64 | vis | 900 | 15.39 | 0.975 | 77.6 | 28.8 | 10321 | 1179 | 19 | 6 |
| 5 | Donkey Kong 64 (USA).z64 | vis | 900 | 15.19 | 0.987 | 63.1 | 26.5 | 5087 | 1389 | 15 | 3 |
| 6 | Mario Party 3 (E) (M4) [!].z64 | vis | 900 | 18.19 | 0.989 | 74.8 | 46.4 | 7843 | 1485 | 18 | 3 |

Compare: `python scripts/chain_compare.py 2026-10-01_wii_audio_full2 <other id or run dir>`
