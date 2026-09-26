#!/usr/bin/env python3
"""Keep repeated ROM entries when comparing baselines."""

from chain_compare import ROOT, load

for source in ("2026-09-25_hw_glN64_full", str(ROOT / "baselines/2026-09-25_hw_glN64_full")):
    rows = load(source)
    assert rows["Super Mario 64.v64"]["vis"] == 3600
    assert rows["Super Mario 64.v64 [run 2]"]["vis"] == 7200
print("duplicate ROM entries preserved")
