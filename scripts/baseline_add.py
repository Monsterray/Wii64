#!/usr/bin/env python3
"""baseline_add.py PERF_LOG --id ID --purpose TEXT [--rom NAME] [--build REV] [--force]
Files a captured perf.log as a baseline under baselines/<ID>/ (perf.log + notes.md) and
appends a row to baselines/index.csv. See baselines/README.md. Reuses perf_compare.py's
page-log parsing so the index/notes numbers are exactly what perf_compare.py would show.
"""
import argparse, csv, datetime, pathlib, shutil, sys

sys.path.insert(0, str(pathlib.Path(__file__).parent))
from perf_compare import pages, summarize

ROOT = pathlib.Path(__file__).parent.parent
BASELINES = ROOT / "baselines"
INDEX = BASELINES / "index.csv"
FIELDS = ["id", "date", "rom", "build", "purpose", "pages",
          "avg_tile_init_us", "avg_tile_load_us", "avg_tile_flush_us",
          "avg_page_total_us", "avg_page_invalidate_us"]

def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("perf_log")
    ap.add_argument("--id", required=True)
    ap.add_argument("--purpose", required=True)
    ap.add_argument("--rom", default="")
    ap.add_argument("--build", default="")
    ap.add_argument("--force", action="store_true")
    args = ap.parse_args()

    dest_dir = BASELINES / args.id
    if dest_dir.exists() and not args.force:
        sys.exit(f"{dest_dir} already exists -- pass --force to replace it")
    dest_dir.mkdir(parents=True, exist_ok=True)

    pgs = pages(args.perf_log)
    if not pgs:
        sys.exit("no complete page-load events in this perf.log -- nothing to file")
    stats = summarize(pgs)

    shutil.copy(args.perf_log, dest_dir / "perf.log")

    row = {
        "id": args.id, "date": datetime.date.today().isoformat(),
        "rom": args.rom, "build": args.build, "purpose": args.purpose,
        **{k: f"{v:.1f}" if isinstance(v, float) else v for k, v in stats.items()},
    }

    notes = dest_dir / "notes.md"
    with open(notes, "w", encoding="utf-8") as f:
        f.write(f"# {args.id}\n\n")
        f.write(f"- date: {row['date']}\n")
        if args.rom: f.write(f"- rom: {args.rom}\n")
        if args.build: f.write(f"- build: {args.build}\n")
        f.write(f"- purpose: {args.purpose}\n\n")
        f.write("## Page-load averages\n\n")
        for k in FIELDS[5:]:
            f.write(f"- {k}: {row[k]}\n")
        f.write(f"\nCompare: `python scripts/perf_compare.py baselines/{args.id}/perf.log <new capture>.log`\n")

    is_new = not INDEX.exists()
    existing = []
    if INDEX.exists():
        with open(INDEX, newline="", encoding="utf-8") as f:
            existing = [r for r in csv.DictReader(f) if r["id"] != args.id]
    existing.append(row)
    with open(INDEX, "w", newline="", encoding="utf-8") as f:
        w = csv.DictWriter(f, fieldnames=FIELDS)
        w.writeheader()
        w.writerows(existing)

    print(f"filed {dest_dir} ({'new' if is_new else 'updated'} index.csv row)")

if __name__ == "__main__":
    main()
