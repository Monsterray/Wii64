# 2026-10-01_dolphin-mk64-race-1.6.6

- date: 2026-10-01
- platform: dolphin
- plugin: glN64
- build: 1.6.6-precommit
- purpose: Wii64 1.6.6 default hash dispatch + RDP reset; warm controller race replay; completed 5400 VIs with MMU/LLE, host playback muted
- screenshots: baselines/media/2026-10-01_dolphin-mk64-race-1.6.6/ (not in git)

| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | Super Mario 64.v64 | vis | 600 | 10.01 | 0.999 | 61.6 | 22.2 | 3755 | 927 | 14 | 7 |
| 2 | Mario Kart 64.v64 | vis | 5400 | 91.43 | 0.984 | 18.3 | 28.0 | 39900 | 1734 | 14 | 126 |

Compare: `python scripts/chain_compare.py 2026-10-01_dolphin-mk64-race-1.6.6 <other id or run dir>`
