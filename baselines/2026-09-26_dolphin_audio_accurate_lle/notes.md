# 2026-09-26_dolphin_audio_accurate_lle

- date: 2026-09-26
- platform: dolphin
- plugin: glN64
- build: 0a05807+dirty
- purpose: SM64 title, 3600 VI, Accurate audio, DSP LLE
- screenshots: baselines/media/2026-09-26_dolphin_audio_accurate_lle/ (not in git)

| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | Super Mario 64.v64 | vis | 3600 | 60.06 | 0.999 | 42.3 | 28.5 | 27294 | 1567 | 15 | 7 |

Compare: `python scripts/chain_compare.py 2026-09-26_dolphin_audio_accurate_lle <other id or run dir>`
