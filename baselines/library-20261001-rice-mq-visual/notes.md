# library-20261001-rice-mq-visual

- date: 2026-10-01
- platform: dolphin
- plugin: Rice
- build: 192be2b+version-1.6.7
- purpose: Master Quest 9000 VI renderer comparison; grey Navi dialogue; XFB RAM copies; full probes
- screenshots: baselines/media/library-20261001-rice-mq-visual/ (not in git)

| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | Zelda Ocarina of Time Master Quest.v64 | vis | 9000 | 151.42 | 0.991 | 46.9 | 28.8 | 161336 | 2455 | 12 | 90 |

Compare: `python scripts/chain_compare.py library-20261001-rice-mq-visual <other id or run dir>`
