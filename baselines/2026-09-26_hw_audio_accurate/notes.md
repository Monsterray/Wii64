# 2026-09-26_hw_audio_accurate

- date: 2026-09-26
- platform: hardware
- plugin: glN64
- build: 0a05807+dirty
- purpose: SM64 title, 3600 VI, Accurate audio, DSP probe
- screenshots: baselines/media/2026-09-26_hw_audio_accurate/ (not in git)

| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | Super Mario 64.v64 | vis | 3600 | 60.26 | 0.996 | 45.2 | 28.2 | 27302 | 1567 | 15 | 13 |

Compare: `python scripts/chain_compare.py 2026-09-26_hw_audio_accurate <other id or run dir>`
