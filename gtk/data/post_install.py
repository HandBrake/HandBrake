#!/usr/bin/env python3

import os
import subprocess
import sys

datadir = sys.argv[1]

# Packaging tools define DESTDIR and this isn't needed for them
if 'DESTDIR' not in os.environ:
    print('Updating icon cache...')
    subprocess.call(['gtk-update-icon-cache', '-tf',
                     os.path.join(datadir, 'icons', 'hicolor')])

    print('Updating desktop database...')
    subprocess.call(['update-desktop-database',
                     os.path.join(datadir, 'applications')])
