# 2026-10-02_hw_jr_lookup_full2

- date: 2026-10-02
- platform: hardware
- plugin: glN64
- build: cdea307+wip
- purpose: JR/JALR target lookup in compiled code + invalidation page walk (full2; control = both off; audio/compile changes in all three)
- screenshots: baselines/media/2026-10-02_hw_jr_lookup_full2/ (not in git)

| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | Super Mario 64.v64 | vis | 2400 | 40.23 | 0.994 | 65.4 | 28.0 | 14188 | 1572 | 14 | 13 |
| 2 | Mario Kart 64.v64 | vis | 3600 | 60.25 | 0.996 | 56.1 | 27.5 | 25069 | 1504 | 14 | 9 |
| 3 | Banjo-Kazooie.V64 | vis | 900 | 15.39 | 0.975 | 48.1 | 26.6 | 5450 | 2083 | 31 | 3 |
| 4 | Pokemon Snap.rom | vis | 900 | 15.25 | 0.984 | 24.2 | 54.3 | 10823 | 777 | 12 | 0 |
| 5 | Super Smash Bros. (U) [!].z64 | vis | 900 | 15.17 | 0.989 | 46.0 | 54.5 | 15948 | 853 | 14 | 0 |
| 6 | GoldenEye 007 (USA).z64 | vis | 900 | 15.18 | 0.988 | 71.7 | 52.4 | 5538 | 837 | 15 | 4 |
| 7 | Wave Race 64 - Kawasaki Jet Ski (USA) (Rev 1).z64 | vis | 900 | 15.23 | 0.985 | 46.6 | 17.1 | 6648 | 866 | 12 | 8 |
| 8 | Diddy Kong Racing (USA) (En,Fr) (Rev 1).z64 | vis | 900 | 15.16 | 0.989 | 71.1 | 26.2 | 6303 | 946 | 13 | 0 |
| 9 | Donkey Kong 64 (USA).z64 | vis | 900 | 15.27 | 0.982 | 62.3 | 26.5 | 5086 | 1389 | 15 | 3 |

Compare: `python scripts/chain_compare.py 2026-10-02_hw_jr_lookup_full2 <other id or run dir>`
