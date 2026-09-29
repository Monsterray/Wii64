# 2026-09-26_hw_audio_smash_fast

- date: 2026-09-26
- platform: hardware
- plugin: glN64
- build: 0a05807+dirty
- purpose: Smash Bros. title, 3600 VI, Fast audio, overrun check
- screenshots: baselines/media/2026-09-26_hw_audio_smash_fast/ (not in git)

| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | Super Smash Bros. (U) [!].z64 | vis | 3600 | 60.22 | 0.996 | 46.9 | 53.5 | 48941 | 2125 | 14 | 1 |

Compare: `python scripts/chain_compare.py 2026-09-26_hw_audio_smash_fast <other id or run dir>`
