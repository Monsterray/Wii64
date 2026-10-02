# 2026-10-02_hw_jr_lookup_final_full1

- date: 2026-10-02
- platform: hardware
- plugin: glN64
- build: cdea307+wip
- purpose: JR/JALR lookup + invalidation page walk with the short store path (full1; control = both off)
- screenshots: baselines/media/2026-10-02_hw_jr_lookup_final_full1/ (not in git)

| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | Super Mario 64.v64 | vis | 2400 | 40.20 | 0.995 | 66.6 | 28.0 | 14190 | 1572 | 14 | 14 |
| 2 | Mario Kart 64.v64 | vis | 3600 | 60.25 | 0.996 | 57.6 | 27.6 | 25068 | 1504 | 14 | 9 |
| 3 | Banjo-Kazooie.V64 | vis | 900 | 15.23 | 0.985 | 49.8 | 26.6 | 5450 | 2083 | 31 | 2 |
| 4 | Pokemon Snap.rom | vis | 900 | 15.23 | 0.985 | 25.5 | 54.4 | 10823 | 777 | 12 | 0 |
| 5 | Super Smash Bros. (U) [!].z64 | vis | 900 | 15.18 | 0.988 | 47.4 | 54.4 | 15800 | 849 | 14 | 0 |
| 6 | GoldenEye 007 (USA).z64 | vis | 900 | 15.18 | 0.988 | 72.5 | 52.4 | 5543 | 837 | 15 | 4 |
| 7 | Wave Race 64 - Kawasaki Jet Ski (USA) (Rev 1).z64 | vis | 900 | 15.22 | 0.985 | 48.3 | 17.1 | 6648 | 866 | 12 | 8 |
| 8 | Diddy Kong Racing (USA) (En,Fr) (Rev 1).z64 | vis | 900 | 15.16 | 0.990 | 72.3 | 26.3 | 6298 | 947 | 13 | 0 |
| 9 | Donkey Kong 64 (USA).z64 | vis | 900 | 15.19 | 0.987 | 63.9 | 26.5 | 5085 | 1389 | 15 | 3 |

Compare: `python scripts/chain_compare.py 2026-10-02_hw_jr_lookup_final_full1 <other id or run dir>`
