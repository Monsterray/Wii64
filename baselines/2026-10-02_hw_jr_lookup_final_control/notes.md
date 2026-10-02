# 2026-10-02_hw_jr_lookup_final_control

- date: 2026-10-02
- platform: hardware
- plugin: glN64
- build: cdea307+wip
- purpose: JR/JALR lookup + invalidation page walk with the short store path (control; control = both off)
- screenshots: baselines/media/2026-10-02_hw_jr_lookup_final_control/ (not in git)

| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | Super Mario 64.v64 | vis | 2400 | 40.21 | 0.995 | 63.4 | 28.0 | 14188 | 1572 | 14 | 10 |
| 2 | Mario Kart 64.v64 | vis | 3600 | 60.24 | 0.996 | 54.7 | 27.6 | 25085 | 1504 | 14 | 8 |
| 3 | Banjo-Kazooie.V64 | vis | 900 | 15.35 | 0.977 | 45.4 | 26.6 | 5450 | 2084 | 31 | 3 |
| 4 | Pokemon Snap.rom | vis | 900 | 15.25 | 0.984 | 20.8 | 54.3 | 10824 | 777 | 12 | 0 |
| 5 | Super Smash Bros. (U) [!].z64 | vis | 900 | 15.17 | 0.989 | 43.4 | 54.5 | 15822 | 849 | 14 | 0 |
| 6 | GoldenEye 007 (USA).z64 | vis | 900 | 15.17 | 0.989 | 70.5 | 52.4 | 5543 | 837 | 15 | 4 |
| 7 | Wave Race 64 - Kawasaki Jet Ski (USA) (Rev 1).z64 | vis | 900 | 15.23 | 0.985 | 45.7 | 17.1 | 6595 | 866 | 12 | 9 |
| 8 | Diddy Kong Racing (USA) (En,Fr) (Rev 1).z64 | vis | 900 | 15.16 | 0.990 | 70.3 | 26.3 | 6299 | 946 | 13 | 0 |
| 9 | Donkey Kong 64 (USA).z64 | vis | 900 | 15.23 | 0.985 | 61.7 | 26.5 | 5086 | 1389 | 15 | 4 |

Compare: `python scripts/chain_compare.py 2026-10-02_hw_jr_lookup_final_control <other id or run dir>`
