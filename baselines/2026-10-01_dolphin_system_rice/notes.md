# 2026-10-01_dolphin_system_rice

- date: 2026-10-01
- platform: dolphin
- plugin: Rice
- build: 76a86b6+system-probes-1.6.5
- purpose: Eight-ROM Rice intro/replay survey; MMU and LLE, host muted, guest audio active. Validated loads/replays and latest boot; PAL Parties match Wii files.
- screenshots: baselines/media/2026-10-01_dolphin_system_rice/ (not in git)

| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | Super Mario 64.v64 | vis | 2400 | 40.01 | 1.000 | 61.5 | 28.0 | 14189 | 1572 | 14 | 9 |
| 2 | Mario Kart 64.v64 | vis | 2400 | 40.01 | 1.000 | 63.4 | 27.6 | 15515 | 686 | 14 | 3 |
| 3 | Banjo-Kazooie.V64 | vis | 900 | 15.32 | 0.979 | 49.8 | 26.6 | 5450 | 2083 | 31 | 3 |
| 4 | Mario Party.v64 | vis | 900 | 15.05 | 0.997 | 68.2 | 55.0 | 7669 | 1420 | 25 | 5 |
| 5 | Mario Party 2 (E) (M5) [!].z64 | vis | 900 | 18.01 | 0.999 | 80.2 | 47.1 | 8401 | 1258 | 17 | 3 |
| 6 | Mario Party 3 (E) (M4) [!].z64 | vis | 900 | 18.00 | 1.000 | 73.6 | 46.5 | 7858 | 1485 | 18 | 1 |
| 7 | Pokemon Snap.rom | vis | 900 | 15.10 | 0.994 | 32.3 | 54.5 | 10823 | 777 | 12 | 0 |
| 8 | Super Smash Bros. (U) [!].z64 | vis | 900 | 15.04 | 0.998 | 47.5 | 54.7 | 15910 | 856 | 14 | 0 |

Compare: `python scripts/chain_compare.py 2026-10-01_dolphin_system_rice <other id or run dir>`
