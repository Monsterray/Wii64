# loading-progress-20261001-dolphin

- date: 2026-10-01
- platform: dolphin
- plugin: glN64
- build: 192be2b+loading-ui-1.6.7
- purpose: New pagefile and controller progress; VM then cached ROM; full probes; no Dolphin faults
- screenshots: baselines/media/loading-progress-20261001-dolphin/ (not in git)

| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | Mario Party (USA).z64 | vis | 600 | 10.01 | 0.999 | 83.9 | 52.1 | 5228 | 810 | 14 | 1 |
| 2 | Super Mario 64.v64 | vis | 600 | 10.01 | 0.999 | 61.6 | 22.6 | 3756 | 927 | 14 | 6 |

Compare: `python scripts/chain_compare.py loading-progress-20261001-dolphin <other id or run dir>`
