# 2026-10-02_hw_jr_lookup_control

- date: 2026-10-02
- platform: hardware
- plugin: glN64
- build: cdea307+wip
- purpose: JR/JALR target lookup in compiled code + invalidation page walk (control; control = both off; audio/compile changes in all three)
- screenshots: baselines/media/2026-10-02_hw_jr_lookup_control/ (not in git)

| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | Super Mario 64.v64 | vis | 2400 | 40.21 | 0.995 | 63.7 | 28.0 | 14186 | 1572 | 14 | 13 |
| 2 | Mario Kart 64.v64 | vis | 3600 | 60.25 | 0.996 | 54.7 | 27.6 | 25069 | 1504 | 14 | 8 |
| 3 | Banjo-Kazooie.V64 | vis | 900 | 15.36 | 0.977 | 45.4 | 26.6 | 5450 | 2084 | 31 | 2 |
| 4 | Pokemon Snap.rom | vis | 900 | 15.25 | 0.983 | 20.6 | 54.3 | 10823 | 777 | 12 | 0 |
| 5 | Super Smash Bros. (U) [!].z64 | vis | 900 | 15.16 | 0.990 | 44.1 | 54.5 | 16098 | 853 | 14 | 0 |
| 6 | GoldenEye 007 (USA).z64 | vis | 900 | 15.18 | 0.988 | 70.5 | 52.4 | 5543 | 837 | 15 | 4 |
| 7 | Wave Race 64 - Kawasaki Jet Ski (USA) (Rev 1).z64 | vis | 900 | 15.23 | 0.985 | 45.4 | 17.1 | 6647 | 866 | 12 | 10 |
| 8 | Diddy Kong Racing (USA) (En,Fr) (Rev 1).z64 | vis | 900 | 15.19 | 0.987 | 70.0 | 26.2 | 6304 | 946 | 13 | 1 |
| 9 | Donkey Kong 64 (USA).z64 | vis | 900 | 15.24 | 0.984 | 61.7 | 26.5 | 5087 | 1389 | 15 | 4 |

Compare: `python scripts/chain_compare.py 2026-10-02_hw_jr_lookup_control <other id or run dir>`
