#!/usr/bin/env python3
"""Check external Steam libraries and paths containing spaces without a game launch."""
import json
import os
import pathlib
import shlex
import subprocess
import tempfile
import prepare_layer

ROOT = pathlib.Path(__file__).resolve().parents[2]


def windows_manifest_checks(base):
    """Check Windows file names and absolute paths without loading Windows DLLs."""
    fake = base / 'Windows checkout'
    (fake / 'work').mkdir(parents=True)
    (fake / 'src/layer').mkdir(parents=True)
    (fake / 'src/layer/VkLayer_dlss_nr.json').write_bytes(
        (ROOT / 'src/layer/VkLayer_dlss_nr.json').read_bytes())
    old_root = prepare_layer.ROOT
    prepare_layer.ROOT = fake
    try:
        (fake / 'work/nr_layer.dll').touch()
        destination = base / 'Windows manifests'
        written = prepare_layer.prepare(destination, 'windows')
        assert written == [destination / 'VkLayer_dlss_nr.json']
        manifest = json.loads((destination / 'VkLayer_dlss_nr.json').read_text())
        assert manifest['layer']['library_arch'] == '64'
        assert manifest['layer']['library_path'] == str(fake / 'work/nr_layer.dll')
        assert manifest['layer']['name'] == 'VK_LAYER_dlssnr_intel'
        assert not (destination / 'VkLayer_dlss_nr32.json').exists()
        (fake / 'work/nr_layer32.dll').touch()
        written = prepare_layer.prepare(destination, 'windows')
        assert len(written) == 2
        manifest = json.loads((destination / 'VkLayer_dlss_nr32.json').read_text())
        assert manifest['layer']['library_arch'] == '32'
        assert manifest['layer']['library_path'] == str(fake / 'work/nr_layer32.dll')
        # A Windows DLL must not become a Linux manifest when the platform is explicit.
        linux = base / 'Linux manifests without SOs'
        assert prepare_layer.prepare(linux, 'linux') == []
        assert not list(linux.glob('*.json'))
    finally:
        prepare_layer.ROOT = old_root


def main():
    with tempfile.TemporaryDirectory(prefix='nr-launcher-') as temporary:
        base = pathlib.Path(temporary)
        windows_manifest_checks(base)
        steam = base / 'Steam root'
        library = base / 'External game library'
        (steam / 'steamapps').mkdir(parents=True)
        prefix = library / 'steamapps/compatdata/311730'
        prefix.mkdir(parents=True)
        proton = library / 'steamapps/common/Proton - Test/proton'
        proton.parent.mkdir(parents=True)
        proton.write_text('#!/bin/sh\nexit 99\n')
        proton.chmod(0o755)
        game = library / 'steamapps/common/Dead or Alive 5/game.exe'
        game.parent.mkdir(parents=True)
        game.touch()
        (steam / 'steamapps/libraryfolders.vdf').write_text(f'"path" "{library}"\n')
        layer = base / 'layers with spaces'
        env = os.environ | {'STEAM_ROOT': str(steam), 'NR_LAYER_DIR': str(layer)}
        env.pop('NR_PROTON', None)
        command = [str(ROOT/'src/layer/nr-photo'), '--check-proton', '311730', str(game)]
        result = subprocess.run(command, env=env, capture_output=True, text=True, check=True)
        assert str(proton) in result.stdout and str(prefix) in result.stdout
        for filename, arch in (('VkLayer_dlss_nr.json', '64'), ('VkLayer_dlss_nr32.json', '32')):
            manifest = json.loads((layer / filename).read_text())
            assert manifest['layer']['library_arch'] == arch
            assert pathlib.Path(manifest['layer']['library_path']).is_file()
        override = base / 'Custom Proton'
        override.write_text('#!/bin/sh\nexit 98\n')
        override.chmod(0o755)
        result = subprocess.run(command, env=env | {'NR_PROTON': str(override)},
                                capture_output=True, text=True, check=True)
        assert str(override) in result.stdout
        # A real direct launch (with a stand-in Proton) must supply the same app IDs
        # Steam normally supplies. A prefix path alone is not that environment.
        override.write_text('#!/usr/bin/env python3\nimport json, os\n'
                            'print(json.dumps({k: os.environ.get(k) for k in '
                            '("SteamAppId", "SteamGameId", "STEAM_COMPAT_APP_ID")}))\n')
        direct = [str(ROOT/'src/layer/nr-photo'), '--proton', '311730', str(game)]
        result = subprocess.run(direct, env=env | {'NR_PROTON': str(override)},
                                capture_output=True, text=True, check=True)
        ids = json.loads(result.stdout.splitlines()[-1])
        assert set(ids.values()) == {'311730'}, ids
        missing = subprocess.run(command[:-1] + [str(base/'missing.exe')], env=env,
                                 capture_output=True, text=True)
        assert missing.returncode != 0 and 'not found' in missing.stderr
        result = subprocess.run([str(ROOT/'src/layer/nr-photo'), '--steam', '311730'],
                                env=env, capture_output=True, text=True, check=True)
        option = next(line.strip() for line in result.stdout.splitlines() if '%command%' in line)
        words = shlex.split(option)
        assert words[0] == f'VK_LAYER_PATH={layer}' and words[-1] == '%command%'
    print('launcher: external library, spaces, Proton override and Linux/Windows manifests OK')


if __name__ == '__main__':
    main()
