# 2026-09-30_wii_paging_long

- date: 2026-09-30
- platform: hardware
- plugin: glN64
- build: 1.6.4+d4066420
- purpose: Paging defaults, longer scenes with staged controller replays; all VI targets and HBC return passed
- screenshots: baselines/media/2026-09-30_wii_paging_long/ (not in git)

| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | Super Mario 64.v64 | vis | 2400 | 40.19 | 0.995 | 64.9 | 27.9 | 14189 | 1572 | 14 | 11 |
| 2 | Mario Party 3 (E) (M4) [!].z64 | vis | 3600 | 72.24 | 0.997 | 73.4 | 69.5 | 25359 | 2347 | 20 | 1 |
| 3 | 007 - The World Is Not Enough (USA).z64 | vis | 3600 | 60.29 | 0.995 | 76.7 | 29.5 | 38068 | 2756 | 25 | 7 |
| 4 | Donkey Kong 64 (USA).z64 | vis | 3600 | 60.25 | 0.996 | 45.8 | 28.9 | 18335 | 1780 | 17 | 4 |
| 5 | Super Mario 64.v64 | vis | 900 | 15.18 | 0.988 | 53.0 | 25.5 | 5981 | 928 | 14 | 9 |

Compare: `python scripts/chain_compare.py 2026-09-30_wii_paging_long <other id or run dir>`
