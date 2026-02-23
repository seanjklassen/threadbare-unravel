import json
import os
import shutil
import sys
from glob import glob


def copy_dir(src: str, dst: str) -> None:
    if not os.path.exists(src):
        raise FileNotFoundError(src)
    if os.path.exists(dst):
        shutil.rmtree(dst)
    shutil.copytree(src, dst)


def find_first(pattern: str) -> str:
    matches = [p for p in glob(pattern, recursive=True) if os.path.isdir(p)]
    if not matches:
        return ""
    matches.sort(key=os.path.getmtime, reverse=True)
    return matches[0]


def resolve_path(raw_path: str, build_dir: str, kind: str) -> str:
    if raw_path and "$<" not in raw_path and os.path.exists(raw_path):
        return raw_path
    if not build_dir:
        return ""
    if kind == "vst3":
        return find_first(os.path.join(build_dir, "**", "*.vst3"))
    if kind == "au":
        return find_first(os.path.join(build_dir, "**", "*.component"))
    return ""


def main() -> int:
    if len(sys.argv) not in (3, 4):
        print("Usage: copy-artefacts.py <artefacts.json> <staging_dir> [build_dir]")
        return 1

    with open(sys.argv[1], "r", encoding="utf-8") as fh:
        artefacts = json.load(fh)

    staging = sys.argv[2]
    build_dir = sys.argv[3] if len(sys.argv) == 4 else ""
    os.makedirs(staging, exist_ok=True)
    vst3_path = resolve_path(artefacts.get("vst3", ""), build_dir, "vst3")
    if not vst3_path:
        raise RuntimeError("Missing 'vst3' artefact path.")
    copy_dir(vst3_path, os.path.join(staging, "VST3", os.path.basename(vst3_path)))

    au_path = resolve_path(artefacts.get("au", ""), build_dir, "au")
    if au_path:
        copy_dir(au_path, os.path.join(staging, "AU", os.path.basename(au_path)))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
