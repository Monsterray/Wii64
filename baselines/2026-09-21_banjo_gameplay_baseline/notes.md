# 2026-09-21_banjo_gameplay_baseline

- date: 2026-09-21
- rom: Banjo-Kazooie
- purpose: 20s autoboot gameplay, dynarec, stock settings

## Captured metrics

- vis_samples: 34
- avg_vis: 58.1
- min_vis: 0.2
- fps_samples: 29
- avg_fps: 26.9
- min_fps: 0.2
- total_exceptions: 5944
- total_cache_resets: 1

Compare: `python scripts/perf_compare.py baselines/2026-09-21_banjo_gameplay_baseline/perf.log <new capture>.log`
