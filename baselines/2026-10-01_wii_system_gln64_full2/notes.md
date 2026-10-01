# 2026-10-01_wii_system_gln64_full2

- date: 2026-10-01
- platform: hardware
- plugin: glN64
- build: 76a86b6+full-system-probes-1.6.5
- purpose: Matched full/control/full system survey, frozen source and chain; original per-call invalidation probe. SM64 replay; Parties A pulses; others intro/title. Mario Kart black/low-work, excluded from healthy gameplay claims.
- screenshots: baselines/media/2026-10-01_wii_system_gln64_full2/ (not in git)

| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | Super Mario 64.v64 | vis | 2400 | 40.22 | 0.995 | 62.9 | 27.9 | 14189 | 1572 | 14 | 14 |
| 2 | Mario Kart 64.v64 | vis | 2400 | 40.17 | 0.996 | 97.8 | 1.4 | 5148 | 403 | 14 | 0 |
| 3 | Banjo-Kazooie.V64 | vis | 900 | 15.27 | 0.982 | 45.4 | 27.6 | 5450 | 2084 | 31 | 2 |
| 4 | Mario Party.v64 | vis | 900 | 15.20 | 0.987 | 71.0 | 57.9 | 7672 | 1420 | 25 | 7 |
| 5 | Mario Party 2 (E) (M5) [!].z64 | vis | 900 | 18.17 | 0.991 | 79.5 | 47.0 | 8399 | 1258 | 17 | 3 |
| 6 | Mario Party 3 (E) (M4) [!].z64 | vis | 900 | 18.15 | 0.992 | 74.9 | 46.4 | 7851 | 1485 | 18 | 1 |
| 7 | Pokemon Snap.rom | vis | 900 | 15.25 | 0.983 | 20.2 | 54.3 | 10824 | 777 | 12 | 0 |
| 8 | Super Smash Bros. (U) [!].z64 | vis | 900 | 15.20 | 0.987 | 44.4 | 54.5 | 15786 | 850 | 14 | 0 |

Compare: `python scripts/chain_compare.py 2026-10-01_wii_system_gln64_full2 <other id or run dir>`
