#!/usr/bin/env python3
"""
opendlss_model — the DLL's weight container as an OpenDLSS-NR model directory.

`maanHimself/OpenDLSS-NR` implements the same network bit-exact against captures of the
original on an NVIDIA GPU, and its WebGPU port runs here, on the Arc 140V, through Chromium
(`src/bench/opendlss_reference.py`). That makes it the one reference this project can run
that claims the vendor's own arithmetic. Its model directory is the container this tree
already reads (`hnet_weights.py`): the same 153 records, each a whole layer's packed payload
under the same `blockN.layerM.parameter` name, sliced out of a few stage files and listed in
a `manifest.json`. Nothing is decoded or re-laid out here; the records' bytes are copied as
they are, which is what that loader expects.

The stage split is ours — the manifest only says where each record lies — and keeps every
stage under 16 MiB, well below the 128 MiB a WebGPU storage binding holds by default. Each
record starts 16-byte aligned inside its stage.

Like everything derived from the DLL, the directory this writes is NVIDIA's and stays in
`work/`: it is never committed or published.

    python3 src/tools/opendlss_model.py [work/weights_ht.bin] [work/opendlss-model]
"""
import hashlib
import json
import pathlib
import re
import sys

import hnet_weights

STAGE_LIMIT = 16 << 20
ALIGN = 16


def write(container, destination):
    blob = container.read_bytes()
    records = hnet_weights.parse(blob)
    stages, tensors = [], []
    current, size = [], 0

    def close():
        nonlocal current, size
        if not current:
            return
        index = len(stages)
        data = bytearray(size)
        for record, offset in current:
            data[offset:offset + record["nbytes"]] = blob[record["offset"]:record["offset"] + record["nbytes"]]
        name = f"stage{index:02d}.bin"
        (destination / "model" / name).write_bytes(data)
        stages.append({"id": f"stage{index:02d}", "file": name, "packedByteLength": len(data),
                       "sha256": hashlib.sha256(data).hexdigest()})
        for record, offset in current:
            block, layer, parameter = re.fullmatch(r"block(\d+)\.layer(\d+)\.(\w+)",
                                                   record["name"]).groups()
            tensors.append({"name": record["name"], "block": int(block), "layer": int(layer),
                            "parameter": parameter, "stage": f"stage{index:02d}",
                            "stageOffset": offset, "byteLength": record["nbytes"]})
        current, size = [], 0

    (destination / "model").mkdir(parents=True, exist_ok=True)
    for record in records:
        start = -(-size // ALIGN) * ALIGN
        if current and start + record["nbytes"] > STAGE_LIMIT:
            close()
            start = 0
        current.append((record, start))
        size = start + record["nbytes"]
    close()

    blocks = {tensor["block"] for tensor in tensors}
    manifest = {"totals": {"blockCount": max(blocks) + 1, "tensorCount": len(tensors)},
                "stages": stages, "tensors": tensors}
    (destination / "manifest.json").write_text(json.dumps(manifest, indent=1) + "\n")
    return manifest


def main():
    root = pathlib.Path(__file__).resolve().parents[2]
    container = pathlib.Path(sys.argv[1]) if len(sys.argv) > 1 else root / "work" / "weights_ht.bin"
    destination = pathlib.Path(sys.argv[2]) if len(sys.argv) > 2 else root / "work" / "opendlss-model"
    manifest = write(container, destination)
    total = sum(stage["packedByteLength"] for stage in manifest["stages"])
    print(f"{destination}: {len(manifest['tensors'])} tensors in {len(manifest['stages'])} stages, "
          f"{total / 1048576:.1f} MiB, {manifest['totals']['blockCount']} blocks")


if __name__ == "__main__":
    main()
