#!/usr/bin/env python3
"""baseline_add.py -- file a run under baselines/ so later runs are compared, not remembered.

Single perf.log (a boxart page load, or a killed gameplay capture):
    baseline_add.py PERF_LOG --id ID --purpose TEXT [--rom NAME] [--build REV] [--force]
  -> baselines/<ID>/perf.log + notes.md, one row in baselines/index.csv.

Chained run (diag.cfg chain=, from .dev/wii64_diag.sh chain or a Wii's SD card):
    baseline_add.py --chain DIR --id ID --purpose TEXT --plugin glN64|Rice
                    [--platform dolphin|hardware] [--build REV] [--force]
  -> baselines/<ID>/{perf.log,diag.cfg,run.log,padtrace_NN.csv,notes.md}, the per-game
     screenshots (xfb_NN.png) under baselines/media/<ID>/ (kept out of git, like
     WiiStation's baselines/media/), and one row PER GAME in baselines/games.csv -- so a
     regression in one game is a diff of two rows. Compare two chained baselines with
     scripts/chain_compare.py.

See baselines/README.md for what the numbers are and are not.
"""
import argparse, csv, datetime, pathlib, re, shutil, subprocess, sys

sys.path.insert(0, str(pathlib.Path(__file__).parent))
from perf_compare import summarize, KEYS
from chain_table import games, xfb_to_png

ROOT = pathlib.Path(__file__).parent.parent
BASELINES = ROOT / "baselines"
INDEX = BASELINES / "index.csv"
GAMES = BASELINES / "games.csv"
FIELDS = ["id", "date", "rom", "build", "purpose"] + KEYS
GAME_FIELDS = ["id", "date", "platform", "plugin", "build", "n", "rom", "how", "vis", "vi_rate", "wall_s",
               "speed", "idle_pct", "avg_vis", "avg_fps", "exceptions", "cacheResets", "recompiles",
               "batches", "verts", "texStalls", "treeDepthMax", "underruns", "overruns",
               "pmc1", "pmc2", "ipc", "heap_used", "heap_free", "arena1_free", "arena2_free",
               "flushes", "flush_us", "padtrace", "purpose"]


def upsert(path, fields, rows, drop_id):
    existing = []
    if path.exists():
        with open(path, newline="", encoding="utf-8") as f:
            existing = [r for r in csv.DictReader(f) if r["id"] != drop_id]
    existing += rows
    with open(path, "w", newline="", encoding="utf-8") as f:
        w = csv.DictWriter(f, fieldnames=fields, extrasaction="ignore")
        w.writeheader()
        w.writerows(existing)


def git_rev():
    try:
        rev = subprocess.check_output(["git", "rev-parse", "--short", "HEAD"], cwd=ROOT, text=True).strip()
        dirty = subprocess.call(["git", "diff", "--quiet"], cwd=ROOT) != 0
        return rev + ("+dirty" if dirty else "")
    except Exception:
        return ""


def file_single(args, dest):
    stats = summarize(args.perf_log)
    if not any(stats[k] for k in KEYS):
        sys.exit("no page-load events or vis/fps samples in this perf.log -- nothing to file")
    shutil.copy(args.perf_log, dest / "perf.log")
    row = {"id": args.id, "date": datetime.date.today().isoformat(), "rom": args.rom,
           "build": args.build, "purpose": args.purpose,
           **{k: f"{v:.1f}" if isinstance(v, float) else v for k, v in stats.items()}}
    with open(dest / "notes.md", "w", encoding="utf-8") as f:
        f.write(f"# {args.id}\n\n- date: {row['date']}\n")
        if args.rom: f.write(f"- rom: {args.rom}\n")
        if args.build: f.write(f"- build: {args.build}\n")
        f.write(f"- purpose: {args.purpose}\n\n## Captured metrics\n\n")
        for k in KEYS:
            if stats[k]:
                f.write(f"- {k}: {row[k]}\n")
        f.write(f"\nCompare: `python scripts/perf_compare.py baselines/{args.id}/perf.log <new capture>.log`\n")
    upsert(INDEX, FIELDS, [row], args.id)
    print(f"filed {dest} (index.csv row)")


def file_chain(args, dest):
    src = pathlib.Path(args.chain)
    rows = games(src / "perf.log")
    if not rows:
        sys.exit(f"{src}/perf.log has no game: lines -- nothing to file")
    media = BASELINES / "media" / args.id
    media.mkdir(parents=True, exist_ok=True)
    for name in ["perf.log", "diag.cfg", "run.log"] + sorted(p.name for p in src.glob("padtrace_*.csv")):
        if (src / name).exists():
            shutil.copy(src / name, dest / name)
    for b in sorted(src.glob("xfb_*.bin")):
        xfb_to_png(str(b), str(media / (b.stem + ".png")))

    today = datetime.date.today().isoformat()
    out = []
    for g in rows:
        wall = g.get("wall_us", 0) / 1e6
        rate = g.get("vi_rate") or (50.0 if re.search(r"\((E|Europe|PAL)\)", g["rom"], re.I) else 60.0)
        out.append({
            "id": args.id, "date": today, "platform": args.platform, "plugin": args.plugin,
            "build": args.build, "n": str(g.get("n", "")).split("/")[0], "rom": pathlib.PurePosixPath(g["rom"]).name,
            "how": g.get("how", ""), "vis": g.get("vis", 0), "wall_s": f"{wall:.2f}",
            "speed": f"{g.get('vis', 0) / wall / rate:.3f}" if wall else "",
            "idle_pct": f"{100 * g.get('sleep_us', 0) / g['wall_us']:.1f}" if g.get("wall_us") else "",
            "ipc": f"{g['pmc2'] / g['pmc1']:.3f}" if g.get("pmc1") else "",
            "purpose": args.purpose,
            **{k: g.get(k, "") for k in GAME_FIELDS if k in g and k not in ("n", "rom", "how", "vis")},
            "vi_rate": rate,
        })
    upsert(GAMES, GAME_FIELDS, out, args.id)

    with open(dest / "notes.md", "w", encoding="utf-8") as f:
        f.write(f"# {args.id}\n\n- date: {today}\n- platform: {args.platform}\n- plugin: {args.plugin}\n")
        if args.build: f.write(f"- build: {args.build}\n")
        f.write(f"- purpose: {args.purpose}\n- screenshots: baselines/media/{args.id}/ (not in git)\n\n")
        f.write("| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |\n")
        f.write("|---|---|---|---|---|---|---|---|---|---|---|---|\n")
        for r in out:
            f.write(f"| {r['n']} | {r['rom']} | {r['how']} | {r['vis']} | {r['wall_s']} | {r['speed']} | {r['idle_pct']} | "
                    f"{float(r['avg_fps'] or 0):.1f} | {r['exceptions']} | {r['recompiles']} | {r['treeDepthMax']} | {r['underruns']} |\n")
        f.write(f"\nCompare: `python scripts/chain_compare.py {args.id} <other id or run dir>`\n")
    print(f"filed {dest} ({len(out)} games.csv rows, screenshots in {media})")


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("perf_log", nargs="?")
    ap.add_argument("--chain", help="a chained run's directory")
    ap.add_argument("--id", required=True)
    ap.add_argument("--purpose", required=True)
    ap.add_argument("--rom", default="")
    ap.add_argument("--plugin", default="")
    ap.add_argument("--platform", default="dolphin", choices=["dolphin", "hardware"])
    ap.add_argument("--build", default=None, help="defaults to the current git revision")
    ap.add_argument("--force", action="store_true")
    args = ap.parse_args()
    if bool(args.chain) == bool(args.perf_log):
        sys.exit("give either PERF_LOG or --chain DIR")
    if args.build is None:
        args.build = git_rev()

    dest = BASELINES / args.id
    if dest.exists() and not args.force:
        sys.exit(f"{dest} already exists -- pass --force to replace it")
    dest.mkdir(parents=True, exist_ok=True)
    (file_chain if args.chain else file_single)(args, dest)


if __name__ == "__main__":
    main()
