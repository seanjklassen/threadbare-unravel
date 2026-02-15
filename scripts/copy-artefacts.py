import json
import os
import shutil
import sys


def copy_dir(src: str, dst: str) -> None:
    if not os.path.exists(src):
        raise FileNotFoundError(src)
    if os.path.exists(dst):
        shutil.rmtree(dst)
    shutil.copytree(src, dst)


def main() -> int:
    if len(sys.argv) != 3:
        print("Usage: copy-artefacts.py <artefacts.json> <staging_dir>")
        return 1

    with open(sys.argv[1], "r", encoding="utf-8") as fh:
        artefacts = json.load(fh)

    staging = sys.argv[2]
    os.makedirs(staging, exist_ok=True)
    copy_dir(artefacts["vst3"], os.path.join(staging, "VST3", os.path.basename(artefacts["vst3"])))
    copy_dir(artefacts["au"], os.path.join(staging, "AU", os.path.basename(artefacts["au"])))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
import json
import os
import shutil
import sys


def copy_dir(src, dst):
    if not os.path.exists(src):
        raise FileNotFoundError(f"Source not found: {src}")
    if os.path.exists(dst):
        shutil.rmtree(dst)
    shutil.copytree(src, dst)


def main():
    if len(sys.argv) != 3:
        print("Usage: copy-artefacts.py <artefacts.json> <staging_dir>")
        return 1

    artefacts_path = sys.argv[1]
    staging_dir = sys.argv[2]

    with open(artefacts_path, "r", encoding="utf-8") as handle:
        artefacts = json.load(handle)

    vst3_path = artefacts.get("vst3")
    au_path = artefacts.get("au")

    if not vst3_path or not au_path:
        print("artefacts.json missing required keys: 'vst3', 'au'")
        return 1

    os.makedirs(staging_dir, exist_ok=True)
    vst3_dest = os.path.join(staging_dir, "VST3", os.path.basename(vst3_path))
    au_dest = os.path.join(staging_dir, "AU", os.path.basename(au_path))
    os.makedirs(os.path.dirname(vst3_dest), exist_ok=True)
    os.makedirs(os.path.dirname(au_dest), exist_ok=True)

    copy_dir(vst3_path, vst3_dest)
    copy_dir(au_path, au_dest)
    print(f"Staged VST3: {vst3_dest}")
    print(f"Staged AU: {au_dest}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
