# 2026-10-01_dolphin_audio_survey

- date: 2026-10-01
- platform: dolphin
- plugin: glN64
- build: 76a86b6+system-probes-1.6.5
- purpose: Six-ROM glN64 audio-system title/replay survey; MMU/LLE and host muted. Validated loads/replays and latest boot; not listening or full gameplay verification.
- screenshots: baselines/media/2026-10-01_dolphin_audio_survey/ (not in git)

| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | GoldenEye 007 (USA).z64 | vis | 900 | 15.07 | 0.996 | 65.2 | 52.3 | 5544 | 837 | 15 | 2 |
| 2 | Diddy Kong Racing (USA) (En,Fr) (Rev 1).z64 | vis | 900 | 15.01 | 0.999 | 68.3 | 26.5 | 6300 | 946 | 13 | 0 |
| 3 | Wave Race 64 - Kawasaki Jet Ski (USA) (Rev 1).z64 | vis | 900 | 15.19 | 0.988 | 37.3 | 17.1 | 6595 | 866 | 12 | 10 |
| 4 | 007 - The World Is Not Enough (USA).z64 | vis | 900 | 15.38 | 0.975 | 75.1 | 28.9 | 10310 | 1179 | 19 | 14 |
| 5 | Donkey Kong 64 (USA).z64 | vis | 900 | 15.31 | 0.980 | 58.8 | 26.2 | 5087 | 1389 | 15 | 5 |
| 6 | Mario Party 3 (E) (M4) [!].z64 | vis | 900 | 18.01 | 0.999 | 71.8 | 46.5 | 7858 | 1485 | 18 | 1 |

Compare: `python scripts/chain_compare.py 2026-10-01_dolphin_audio_survey <other id or run dir>`
