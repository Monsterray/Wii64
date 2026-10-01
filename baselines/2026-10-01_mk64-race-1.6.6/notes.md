# 2026-10-01_mk64-race-1.6.6

- date: 2026-10-01
- platform: hardware
- plugin: glN64
- build: 1.6.6-precommit
- purpose: Wii64 1.6.6 default fixed hash + full RDP reset; 22-record replay enters active race after SM64 warm-up; 5400 VIs; HBC return verified
- screenshots: baselines/media/2026-10-01_mk64-race-1.6.6/ (not in git)

| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | Super Mario 64.v64 | vis | 600 | 10.19 | 0.982 | 63.5 | 22.1 | 3755 | 927 | 14 | 9 |
| 2 | Mario Kart 64.v64 | vis | 5400 | 90.85 | 0.991 | 22.6 | 28.3 | 39918 | 1734 | 14 | 59 |

Compare: `python scripts/chain_compare.py 2026-10-01_mk64-race-1.6.6 <other id or run dir>`
