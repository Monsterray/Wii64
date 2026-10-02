# 2026-10-02_hw_page_skip_tlb_control

- date: 2026-10-02
- platform: hardware
- plugin: glN64
- build: cdea307+wip
- purpose: Invalidation page walk with the short store path, TWINE and GoldenEye (control; control = page skip off)
- screenshots: baselines/media/2026-10-02_hw_page_skip_tlb_control/ (not in git)

| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | 007 - The World Is Not Enough (USA).z64 | vis | 1800 | 30.20 | 0.994 | 83.9 | 28.9 | 14887 | 1624 | 25 | 1 |
| 2 | GoldenEye 007 (USA).z64 | vis | 1800 | 30.18 | 0.994 | 71.0 | 54.5 | 11839 | 1080 | 15 | 5 |

Compare: `python scripts/chain_compare.py 2026-10-02_hw_page_skip_tlb_control <other id or run dir>`
