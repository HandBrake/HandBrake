# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (c) 2023 HandBrake Team

# Project sources
ghb_src = files(
  'audiohandler.c',
  'callbacks.c',
  'chapters.c',
  'color-scheme.c',
  'ghb-dvd.c',
  'ghbcompat.c',
  'hb-backend.c',
  'icons.c',
  'jobdict.c',
  'main.c',
  'plist.c',
  'power-manager.c',
  'presets.c',
  'preview.c',
  'queuehandler.c',
  'renderer_button.c',
  'resources.c',
  'settings.c',
  'subtitlehandler.c',
  'title-add.c',
  'titledict.c',
  'values.c',
  'videohandler.c',
)

ui_dir = get_option('gtk4') ? 'ui-gtk4' : 'ui-gtk3'

ui_files = files(
  ui_dir / 'menu.ui',
  ui_dir / 'ghb.ui',
)

config_h = configuration_data()
config_h.set('GHB_FLATPAK', get_option('flatpak'),
  description: 'Define if building HandBrake for Flatpak.')
config_h.set_quoted('GETTEXT_PACKAGE', 'ghb',
  description: 'GETTEXT package name')
config_h.set('ENABLE_NLS', true,
  description: 'Define if translation of program messages is requested.')
config_h.set_quoted('PACKAGE_LOCALE_DIR',
  get_option('prefix') / get_option('localedir'),
  description: 'The directory to install the translation files in.')

configure_file(configuration: config_h, output: 'config.h')

# Generated files

ui = gnome.compile_resources('ui_res', 'ui_res.gresource.xml',
  dependencies: ui_files,
  source_dir: ui_dir,
  extra_args: '--manual-register',
  c_name: 'ghb_ui')

# Build targets
executable('ghb',
  sources: [ghb_src, data, icons, ui, marshalers],
  dependencies: ghb_deps,
  include_directories: hb_incdirs,
  link_language: 'cpp',
  link_args: get_option('c_link_args'),
  install: true)
