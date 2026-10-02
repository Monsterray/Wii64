# 2026-10-02_hw_jr_lookup_full1

- date: 2026-10-02
- platform: hardware
- plugin: glN64
- build: cdea307+wip
- purpose: JR/JALR target lookup in compiled code + invalidation page walk (full1; control = both off; audio/compile changes in all three)
- screenshots: baselines/media/2026-10-02_hw_jr_lookup_full1/ (not in git)

| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | Super Mario 64.v64 | vis | 2400 | 40.21 | 0.995 | 65.5 | 28.0 | 14190 | 1572 | 14 | 12 |
| 2 | Mario Kart 64.v64 | vis | 3600 | 60.25 | 0.996 | 56.1 | 27.6 | 25071 | 1504 | 14 | 9 |
| 3 | Banjo-Kazooie.V64 | vis | 900 | 15.43 | 0.972 | 48.3 | 26.6 | 5450 | 2083 | 31 | 5 |
| 4 | Pokemon Snap.rom | vis | 900 | 15.27 | 0.982 | 24.2 | 54.3 | 10822 | 777 | 12 | 0 |
| 5 | Super Smash Bros. (U) [!].z64 | vis | 900 | 15.18 | 0.988 | 46.1 | 54.4 | 15797 | 847 | 14 | 0 |
| 6 | GoldenEye 007 (USA).z64 | vis | 900 | 15.19 | 0.988 | 71.7 | 52.4 | 5542 | 837 | 15 | 4 |
| 7 | Wave Race 64 - Kawasaki Jet Ski (USA) (Rev 1).z64 | vis | 900 | 15.23 | 0.985 | 47.0 | 17.1 | 6594 | 879 | 12 | 8 |
| 8 | Diddy Kong Racing (USA) (En,Fr) (Rev 1).z64 | vis | 900 | 15.16 | 0.989 | 71.1 | 26.3 | 6303 | 946 | 13 | 0 |
| 9 | Donkey Kong 64 (USA).z64 | vis | 900 | 15.26 | 0.983 | 62.3 | 26.5 | 5087 | 1389 | 15 | 3 |

Compare: `python scripts/chain_compare.py 2026-10-02_hw_jr_lookup_full1 <other id or run dir>`
