# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (c) 2023 HandBrake Team

icon_files = files(
  '24x24/apps/hb-advanced-symbolic.symbolic.png',
  '32x32/apps/hb-source-symbolic.symbolic.png',
  '32x32/apps/hb-picture-symbolic.symbolic.png',
  '32x32/apps/hb-add-queue-symbolic.symbolic.png',
  '32x32/apps/hb-showqueue-symbolic.symbolic.png',
  '32x32/apps/hb-activity-symbolic.symbolic.png',
  '32x32/apps/hb-presets-symbolic.symbolic.png',
  '32x32/apps/hb-start.png',
  '32x32/apps/hb-stop.png',
  '32x32/apps/hb-pause.png',
  '48x48/apps/hb-advanced-symbolic.symbolic.png',
  '64x64/apps/hb-source-symbolic.symbolic.png',
  '64x64/apps/hb-picture-symbolic.symbolic.png',
  '64x64/apps/hb-add-queue-symbolic.symbolic.png',
  '64x64/apps/hb-showqueue-symbolic.symbolic.png',
  '64x64/apps/hb-activity-symbolic.symbolic.png',
  '64x64/apps/hb-presets-symbolic.symbolic.png',
  '64x64/apps/hb-start.png',
  '64x64/apps/hb-stop.png',
  '64x64/apps/hb-pause.png',
  'scalable/apps/hb-advanced-symbolic.svg',
  'scalable/apps/hb-source-symbolic.svg',
  'scalable/apps/hb-picture-symbolic.svg',
  'scalable/apps/hb-add-queue-symbolic.svg',
  'scalable/apps/hb-showqueue-symbolic.svg',
  'scalable/apps/hb-activity-symbolic.svg',
  'scalable/apps/hb-presets-symbolic.svg',
  'scalable/apps/hb-start.svg',
  'scalable/apps/hb-stop.svg',
  'scalable/apps/hb-pause.svg',
  'scalable/apps/hb-complete.svg',
  'scalable/apps/hb-icon.svg',
  'scalable/apps/fr.handbrake.ghb.svg',
)

icons = gnome.compile_resources('icon_res', 'icon_res.gresource.xml',
  dependencies: icon_files,
  extra_args: '--manual-register',
  c_name: 'ghb_icons')

if host_machine.system() not in ['windows', 'haiku']
  install_data('fr.handbrake.ghb.svg',
    install_dir: get_option('datadir') / 'icons/hicolor/scalable/apps')
endif
