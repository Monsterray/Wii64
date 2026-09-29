# 2026-09-26_hw_audio_smash_accurate

- date: 2026-09-26
- platform: hardware
- plugin: glN64
- build: 0a05807+dirty
- purpose: Smash Bros. title, 3600 VI, Accurate audio, overrun check
- screenshots: baselines/media/2026-09-26_hw_audio_smash_accurate/ (not in git)

| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | Super Smash Bros. (U) [!].z64 | vis | 3600 | 60.24 | 0.996 | 45.7 | 53.3 | 48820 | 2127 | 14 | 1 |

Compare: `python scripts/chain_compare.py 2026-09-26_hw_audio_smash_accurate <other id or run dir>`
