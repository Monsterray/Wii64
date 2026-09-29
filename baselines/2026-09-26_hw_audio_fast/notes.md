# 2026-09-26_hw_audio_fast

- date: 2026-09-26
- platform: hardware
- plugin: glN64
- build: 0a05807+dirty
- purpose: SM64 title, 3600 VI, Fast audio, DSP probe
- screenshots: baselines/media/2026-09-26_hw_audio_fast/ (not in git)

| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | Super Mario 64.v64 | vis | 3600 | 60.23 | 0.996 | 46.2 | 28.2 | 27288 | 1567 | 15 | 10 |

Compare: `python scripts/chain_compare.py 2026-09-26_hw_audio_fast <other id or run dir>`
