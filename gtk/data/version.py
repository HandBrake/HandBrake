#!/usr/bin/env python3

import subprocess
import sys
import time

def cmd_out(cmd: str) -> str:
    try:
        process = subprocess.run(cmd.split(), capture_output=True, text=True)
        return process.stdout.splitlines()[0]
    except:
        return ''

def txt_vars() -> dict:
    for path in ['./', '../', '../../']:
        try:
            with open(path + 'version.txt', 'r') as ver:
                lines = ver.read().splitlines()
                vardict = {k: v for k, v in (l.split('=') for l in lines)}
                return vardict
        except:
            pass
    return {}

def print_version():
    git_tag = cmd_out('git describe --tags --exact-match HEAD')
    git_date = cmd_out('git log -1 --date=format:%Y%m%d --format=%cd')
    git_rev = cmd_out('git rev-parse --short HEAD')

    if len(git_tag) > 0:
        print(git_tag)
    elif len(git_date) > 0:
        print(f'{git_date}-git-{git_rev}')
    else:
        vars = txt_vars()
        try:
            txt_tag = vars.get('TAG')
            txt_date = vars.get('DATE')
            txt_rev = vars.get('SHORTHASH')
            if txt_tag is not None:
                print(txt_tag)
            else:
                txt_date = txt_date.split(' ')[0].replace('-', '')
                print(f'{txt_date}-git-{txt_rev}')
        except:
            cur_date = time.strftime('%Y%m%d')
            print(f'{cur_date}-unknown')

def print_date():
    git_date = cmd_out('git log -1 --date=format:%Y-%m-%d --format=%cd')
    if len(git_date) > 0:
        print(git_date)
    else:
        txt_date = txt_vars().get('DATE')
        if txt_date not in [None, '']:
            print(txt_date.split(' ')[0])
        else:
            print(time.strftime('%Y-%m-%d'))

if __name__ == "__main__":
    if len(sys.argv) > 1 and sys.argv[1] == 'date':
        print_date()
    else:
        print_version()

