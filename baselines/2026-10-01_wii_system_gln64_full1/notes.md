# 2026-10-01_wii_system_gln64_full1

- date: 2026-10-01
- platform: hardware
- plugin: glN64
- build: 76a86b6+full-system-probes-1.6.5
- purpose: Matched full/control/full system survey, frozen source and chain; original per-call invalidation probe. SM64 replay; Parties A pulses; others intro/title. Mario Kart black/low-work, excluded from healthy gameplay claims.
- screenshots: baselines/media/2026-10-01_wii_system_gln64_full1/ (not in git)

| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | Super Mario 64.v64 | vis | 2400 | 40.22 | 0.995 | 62.9 | 27.9 | 14189 | 1572 | 14 | 14 |
| 2 | Mario Kart 64.v64 | vis | 2400 | 40.17 | 0.996 | 97.8 | 1.4 | 5147 | 403 | 14 | 0 |
| 3 | Banjo-Kazooie.V64 | vis | 900 | 15.27 | 0.982 | 45.5 | 27.6 | 5450 | 2083 | 31 | 3 |
| 4 | Mario Party.v64 | vis | 900 | 15.21 | 0.986 | 71.0 | 57.9 | 7670 | 1420 | 25 | 7 |
| 5 | Mario Party 2 (E) (M5) [!].z64 | vis | 900 | 18.13 | 0.993 | 79.5 | 47.1 | 8400 | 1258 | 17 | 3 |
| 6 | Mario Party 3 (E) (M4) [!].z64 | vis | 900 | 18.16 | 0.991 | 74.9 | 46.5 | 7845 | 1485 | 18 | 1 |
| 7 | Pokemon Snap.rom | vis | 900 | 15.26 | 0.983 | 20.2 | 56.1 | 10824 | 777 | 12 | 0 |
| 8 | Super Smash Bros. (U) [!].z64 | vis | 900 | 15.20 | 0.987 | 44.2 | 54.5 | 15806 | 848 | 14 | 0 |

Compare: `python scripts/chain_compare.py 2026-10-01_wii_system_gln64_full1 <other id or run dir>`
