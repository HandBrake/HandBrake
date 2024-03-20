# Copyright (C) 2023-2024 HandBrake Team
# SPDX-License-Identifier: GPL-2.0-or-later

icons = gnome.compile_resources('icon_res', 'icon_res.gresource.xml',
  c_name: 'ghb_icons')

if host_machine.system() not in ['windows', 'haiku']
  install_data('scalable/apps/fr.handbrake.ghb.svg',
    install_dir: get_option('datadir') / 'icons/hicolor/scalable/apps')
endif
