# 2026-09-30_dolphin_texture_snapshot

- date: 2026-09-30
- platform: dolphin
- plugin: glN64
- build: 1.6.4+83e7f18f
- purpose: Byte-snapshot hash candidate, LLE MMU muted, all 3 graphics targets passed; Wii gain not yet accepted
- screenshots: baselines/media/2026-09-30_dolphin_texture_snapshot/ (not in git)

| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | Super Mario 64.v64 | vis | 900 | 15.01 | 0.999 | 49.6 | 25.4 | 5981 | 928 | 14 | 6 |
| 2 | Banjo-Kazooie.V64 | vis | 900 | 15.07 | 0.995 | 47.9 | 26.6 | 5450 | 2084 | 31 | 3 |
| 3 | Pokemon Snap.rom | vis | 900 | 15.11 | 0.992 | 19.8 | 54.1 | 10824 | 777 | 12 | 0 |

Compare: `python scripts/chain_compare.py 2026-09-30_dolphin_texture_snapshot <other id or run dir>`
