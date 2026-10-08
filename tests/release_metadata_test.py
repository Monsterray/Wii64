"""All packaged HBC metadata must match the runtime version."""
from datetime import datetime
from pathlib import Path
import re
import xml.etree.ElementTree as ET

root = Path(__file__).resolve().parents[1]
version = re.search(r'#define WII64_VERSION "(\d+\.\d+\.\d+)"',
                    (root / "main/version.h").read_text()).group(1)
files = sorted((root / "release/apps").glob("*/meta.xml"))
assert len(files) == 4, "Check both renderers and their Wii VC variants"
for path in files:
    app = ET.parse(path).getroot()
    assert app.tag == "app" and app.find("ahb_access") is not None, path
    assert app.findtext("version") == version, path
    assert f"Wii64 {version} (" in app.findtext("long_description"), path
    date = app.findtext("release_date")
    assert re.fullmatch(r"\d{12}", date), path
    datetime.strptime(date, "%Y%m%d%H%M")
assert len({ET.parse(path).findtext("release_date") for path in files}) == 1
print(f"Release metadata: four valid HBC XML files match Wii64 {version} PASS")
