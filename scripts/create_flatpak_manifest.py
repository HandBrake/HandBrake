#!/usr/bin/env python3

import types
import os
import sys
import json
import getopt
import posixpath
from collections import OrderedDict
from urllib.parse import urlsplit, unquote


def url2filename(url):
    path = urlsplit(url).path
    return posixpath.basename(unquote(path))

def islocal(url):
    result = urlsplit(url)
    return result.scheme == "" and result.netloc == ""

class SourceType:
    contrib = 1
    archive = 2

class SourceEntry:
    def __init__(self, url, entry_type, sha256=None):
        self.url        = url
        self.entry_type = entry_type
        self.sha256     = sha256

class FlatpakPluginManifest:
    def __init__(self, runtime, template=None):
        if template is not None and os.path.exists(template):
            with open(template, 'r') as fp:
                self.manifest = json.load(fp, object_pairs_hook=OrderedDict)

        else:
            self.manifest    = OrderedDict()

        if runtime != None:
            self.manifest["runtime-version"] = runtime

class FlatpakManifest:
    def __init__(self, source_list, runtime, qsv, vcn, nvenc, dovi, template=None):
        if template is not None and os.path.exists(template):
            with open(template, 'r') as fp:
                self.manifest = json.load(fp, object_pairs_hook=OrderedDict)

            self.finish_args = self.manifest["finish-args"]
            self.modules     = self.manifest["modules"]
            self.hbmodule    = self.modules[len(self.modules) - 1]
            self.hbconfig    = self.hbmodule["config-opts"]
            self.sources     = [None]

            self.hbmodule["sources"]     = self.sources
        else:
            self.manifest    = OrderedDict()
            self.modules     = []
            self.hbmodule    = OrderedDict()
            self.sources     = [None]

            self.manifest["finish-args"] = self.finish_args
            self.manifest["modules"]     = self.modules
            self.modules[0]              = self.hbmodule
            self.hbmodule["sources"]     = self.sources
            self.hbconfig                = [None]

        self.extensions  = []
        self.build_path  = []
        self.build_args  = []
        self.ld_path     = []

        if runtime is not None:
            self.manifest["runtime-version"] = runtime

        if qsv:
            self.hbconfig.append("--enable-qsv");

        if nvenc:
            self.hbconfig.append("--enable-nvenc");
            self.hbconfig.append("--enable-nvdec");
            self.extensions += ['org.freedesktop.Sdk.Extension.llvm18'];
            self.build_path += ['/usr/lib/sdk/llvm18/bin'];
            self.ld_path    += ['/usr/lib/sdk/llvm18/lib'];

        if vcn:
            self.hbconfig.append("--enable-vce");

        if dovi:
            self.hbconfig.append("--enable-libdovi");
            self.extensions += ['org.freedesktop.Sdk.Extension.rust-stable'];
            self.build_path += ['/usr/lib/sdk/rust-stable/bin'];

        if len(self.extensions) > 0:
            self.manifest["sdk-extensions"] = self.extensions

        if len(self.build_path) > 0:
            self.hbmodule["build-options"].update({'append-path': ':'.join(self.build_path)})

        if len(self.build_args) > 0:
            self.hbmodule["build-options"].update({'build-args': self.build_args})

        if self.ld_path:
            self.hbmodule["build-options"].update({'prepend-ld-library-path': ':'.join(self.ld_path)})

        handbrake_found = False
        for key, value in source_list.items():
            source = OrderedDict()
            if islocal(value.url):
                source["path"] = value.url
            else:
                if value.sha256 is None or value.sha256 == "":
                    continue
                source["url"] = value.url
                source["sha256"] = value.sha256

            if value.entry_type == SourceType.archive:
                if handbrake_found:
                    print("Error: only one archive source permitted")
                    sys.exit(3)

                source["type"] = "archive"
                source["strip-components"] = 1
                self.sources[0] = source
                handbrake_found = True

            elif value.entry_type == SourceType.contrib:
                source["type"] = "file"
                source["dest"] = "download"
                if value.basename != None and value.basename != "":
                    source["dest-filename"] = value.basename
                else:
                    source["dest-filename"] = url2filename(value.url)
                self.sources.append(source)


def usage():
    print("create_flatpak_manifest [-a <archive>] [-c <contrib>] [-s <sha265>] [-t <template>] [-r <sdk-runtime-version] [-h] [<dst>]")
    print("     -a --archive    - Main archive (a.k.a. HB sources)")
    print("     -c --contrib    - Contrib download URL (can be repeated)")
    print("     -s --sha256     - sha256 of previous file on command line")
    print("     -b --basename   - target basename of previous file on command line")
    print("     -t --template   - Flatpak manifest template")
    print("     -r --runtime    - Flatpak SDK runtime version")
    print("     -q --qsv        - Build with Intel QSV support")
    print("     -v --vcn        - Build with AMD VCN support")
    print("     -e --nvenc      - Build with Nvidia HW Encoder support")
    print("     -d --dovi       - Build with libdovi support")
    print("     -p --plugin     - Manifest if for a HandBrake flatpak plugin")
    print("     -h --help       - Show this message")

if __name__ == "__main__":
    try:
        opts, args = getopt.getopt(sys.argv[1:], "a:c:s:b:t:r:qvedph",
            ["archive=", "contrib=", "sha256=", "basename=",
             "template=", "runtime=", "qsv", "vcn", "nvenc", "dovi", "plugin", "help"])
    except getopt.GetoptError:
        print("Error: Invalid option")
        usage()
        sys.exit(2)

    if len(args) > 1:
        usage()
        exit(2)

    source_list = OrderedDict()
    current_source = None
    runtime = None
    plugin = 0
    dovi = 0
    qsv = 0
    vcn = 0
    nvenc = 1
    print("ARGS ",args)
    print("OPT ",opts)
    # exit()
    for opt, arg in opts:
        if opt in ("-h", "--help"):
            usage()
            sys.exit()
        elif opt in ("-a", "--archive"):
            if arg is not None and arg != "":
                current_source = arg
                source_list[arg] = SourceEntry(arg, SourceType.archive)
                source_list[current_source].sha256 = None
                source_list[current_source].basename = None
            else:
                current_source = None
        elif opt in ("-c", "--contrib"):
            if arg is not None and arg != "":
                current_source = arg
                source_list[arg] = SourceEntry(arg, SourceType.contrib)
                source_list[current_source].sha256 = None
                source_list[current_source].basename = None
            else:
                current_source = None
        elif opt in ("-s", "--sha256"):
            if current_source is not None:
                source_list[current_source].sha256 = arg
        elif opt in ("-b", "--basename"):
            if current_source is not None:
                source_list[current_source].basename = arg
        elif opt in ("-t", "--template"):
            template = arg
        elif opt in ("-r", "--runtime"):
            runtime = arg
        elif opt in ("-q", "--qsv"):
            qsv = 1;
        elif opt in ("-v", "--vcn"):
            vcn = 1;
        elif opt in ("-e", "--nvenc"):
            print("NVENC ON")
            nvenc = 1;
        elif opt in ("-d", "--dovi"):
            dovi = 1;
        elif opt in ("-p", "--plugin"):
            plugin = 1;

    if len(args) > 0:
        dst = args[0]
    else:
        dst = None

    if plugin:
        manifest = FlatpakPluginManifest(runtime, template)
    else:
        manifest = FlatpakManifest(source_list, runtime, qsv, vcn, nvenc, dovi, template)

    if dst is not None:
        with open(dst, 'w') as fp:
            json.dump(manifest.manifest, fp, ensure_ascii=False, indent=4)
    else:
        print(json.dumps(manifest.manifest, ensure_ascii=False, indent=4))

