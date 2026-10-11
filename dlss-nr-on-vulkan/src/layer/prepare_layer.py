#!/usr/bin/env python3
"""Write local Vulkan manifests for the available 64/32-bit layer libraries."""
import argparse
import json
import sys
import pathlib

ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "src"))
import nr_build  # noqa: E402


# the platform's shared-library suffix, as both builds name them
SUFFIX = nr_build.SUFFIX
_CHECKOUT = ROOT


def library_dir():
    """Where the layer libraries are: the build directory `make` or CMake wrote
    (nr_build.BUILD_DIR), or `work/` under a ROOT a test has pointed at a stand-in tree."""
    return nr_build.BUILD_DIR if ROOT == _CHECKOUT else ROOT / 'work'


def prepare(destination, platform=None):
    platform = platform or ('windows' if sys.platform == 'win32' else 'posix')
    if platform not in ('linux', 'posix', 'windows'):
        raise ValueError('platform must be linux, posix or windows')
    destination = pathlib.Path(destination)
    destination.mkdir(parents=True, exist_ok=True)
    template = json.loads((ROOT / 'src/layer/VkLayer_dlss_nr.json').read_text())
    written = []
    # the MSVC build (tools/build_win.bat) names the layer nr_layer.dll; make and CMake
    # name it libnr_layer with the platform's suffix (nr_build.SUFFIX: .so, .dylib, .dll)
    filenames = (('nr_layer.dll', 'nr_layer32.dll') if platform == 'windows'
                 else ('libnr_layer' + SUFFIX, 'libnr_layer32' + SUFFIX))
    for arch, filename, manifest_name in (
            ('64', filenames[0], 'VkLayer_dlss_nr.json'),
            ('32', filenames[1], 'VkLayer_dlss_nr32.json')):
        library = library_dir() / filename        # where `make` or CMake put it
        if not library.exists():
            continue
        manifest = dict(template, layer=dict(template['layer'], library_path=str(library), library_arch=arch))
        target = destination / manifest_name
        target.write_text(json.dumps(manifest, indent=2) + '\n', encoding='utf-8')
        written.append(target)
    return written


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('destination', type=pathlib.Path)
    parser.add_argument('--platform', choices=('linux', 'posix', 'windows'),
                        help='default: the current operating system')
    args = parser.parse_args()
    written = prepare(args.destination, args.platform)
    if not written:
        parser.exit(1, f'No matching layer library found in {library_dir()}; build it first.\n')
    for path in written:
        print(path)
