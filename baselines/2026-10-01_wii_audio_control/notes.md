# 2026-10-01_wii_audio_control

- date: 2026-10-01
- platform: hardware
- plugin: glN64
- build: 76a86b6+system-control-1.6.5
- purpose: Six-ROM audio-system title/replay survey, probes disabled. Loads, replays and HBC return validated; first full run had a host launcher error, so not a fully validated probe-cost triple.
- screenshots: baselines/media/2026-10-01_wii_audio_control/ (not in git)

| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | GoldenEye 007 (USA).z64 | vis | 900 | 15.18 | 0.988 | 71.5 | 52.4 | 5542 | 837 | 15 | 3 |
| 2 | Diddy Kong Racing (USA) (En,Fr) (Rev 1).z64 | vis | 900 | 15.16 | 0.989 | 74.0 | 26.3 | 6299 | 947 | 13 | 0 |
| 3 | Wave Race 64 - Kawasaki Jet Ski (USA) (Rev 1).z64 | vis | 900 | 15.26 | 0.983 | 48.3 | 17.1 | 6594 | 866 | 12 | 9 |
| 4 | 007 - The World Is Not Enough (USA).z64 | vis | 900 | 15.19 | 0.988 | 79.8 | 28.8 | 10316 | 1179 | 19 | 1 |
| 5 | Donkey Kong 64 (USA).z64 | vis | 900 | 15.19 | 0.988 | 66.3 | 28.1 | 5085 | 1389 | 15 | 3 |
| 6 | Mario Party 3 (E) (M4) [!].z64 | vis | 900 | 18.16 | 0.991 | 76.8 | 46.3 | 7834 | 1485 | 18 | 1 |

Compare: `python scripts/chain_compare.py 2026-10-01_wii_audio_control <other id or run dir>`
