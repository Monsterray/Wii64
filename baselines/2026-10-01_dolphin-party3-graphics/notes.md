# 2026-10-01_dolphin-party3-graphics

- date: 2026-10-01
- platform: dolphin
- plugin: glN64
- build: f115e06+gdp-reset+fixed-hash
- purpose: Isolated Mario Party 3 PAL periodic-A intro check after combined-chain host startup timeout
- screenshots: baselines/media/2026-10-01_dolphin-party3-graphics/ (not in git)

| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | Mario Party 3 (E) (M4) [!].z64 | vis | 900 | 17.99 | 1.000 | 73.4 | 46.4 | 7873 | 1485 | 18 | 1 |

Compare: `python scripts/chain_compare.py 2026-10-01_dolphin-party3-graphics <other id or run dir>`
