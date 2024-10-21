#!/usr/bin/env python3

import os
import subprocess
import sys

datadir = sys.argv[1]

# Packaging tools define DESTDIR and this isn't needed for them
if 'DESTDIR' not in os.environ:
    try:
        print('Updating icon cache...')
        subprocess.run(['gtk-update-icon-cache', '-tf',
                        os.path.join(datadir, 'icons', 'hicolor')])
    except:
        print('gtk-update-icon-cache was not found in PATH.')

    try:
        print('Updating desktop database...')
        subprocess.run(['update-desktop-database',
                        os.path.join(datadir, 'applications')])
    except:
        print('update-desktop-database was not found in PATH.')
