# 2026-09-21_selectrom_baseline

- date: 2026-09-21
- rom: Banjo-Kazooie etc (5 ROMs)
- purpose: post-timestamp-fix baseline, 5-ROM SD roms folder, stress_selectrom=1

## Page-load averages

- pages: 1
- avg_tile_init_us: 57.8
- avg_tile_load_us: 991.6
- avg_tile_flush_us: 16.0
- avg_page_total_us: 11388.0
- avg_page_invalidate_us: 0.0

Compare: `python scripts/perf_compare.py baselines/2026-09-21_selectrom_baseline/perf.log <new capture>.log`
