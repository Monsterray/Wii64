# 2026-09-30_dolphin_subsystems_1.6.2

- date: 2026-09-30
- platform: dolphin
- plugin: glN64
- build: ea7802e+subsystem-probes-1.6.2
- purpose: Final 1.6.2 six neutral scenes, DSP LLE, host playback muted; Dolphin PMCs are not Wii overhead
- screenshots: baselines/media/2026-09-30_dolphin_subsystems_1.6.2/ (not in git)

| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | Super Mario 64.v64 | vis | 900 | 15.01 | 0.999 | 50.1 | 25.4 | 5980 | 928 | 14 | 7 |
| 2 | Banjo-Kazooie.V64 | vis | 900 | 15.07 | 0.995 | 48.6 | 26.7 | 5450 | 2084 | 31 | 2 |
| 3 | Pokemon Snap.rom | vis | 900 | 15.13 | 0.991 | 16.8 | 54.2 | 10824 | 777 | 12 | 1 |
| 4 | Mario Party 3 (E) (M4) [!].z64 | vis | 900 | 20.52 | 0.877 | 60.7 | 48.3 | 7904 | 1286 | 18 | 1 |
| 5 | 007 - The World Is Not Enough (USA).z64 | vis | 900 | 18.16 | 0.826 | 65.0 | 27.8 | 10319 | 1179 | 19 | 19 |
| 6 | Donkey Kong 64 (USA).z64 | vis | 900 | 17.77 | 0.844 | 52.3 | 27.9 | 5084 | 1389 | 15 | 14 |

Compare: `python scripts/chain_compare.py 2026-09-30_dolphin_subsystems_1.6.2 <other id or run dir>`
