# Copyright (C) 2023-2025 HandBrake Team
# SPDX-License-Identifier: GPL-2.0-or-later

# Project sources
ghb_src = files(
  'application.c',
  'audiohandler.c',
  'callbacks.c',
  'chapters.c',
  'color-scheme.c',
  'ghb-button.c',
  'ghb-chapter-row.c',
  'ghb-file-button.c',
  'ghb-queue-row.c',
  'ghb-string-list.c',
  'hb-backend.c',
  'hb-dvd.c',
  'jobdict.c',
  'main.c',
  'notifications.c',
  'power-manager.c',
  'presets.c',
  'preview.c',
  'queuehandler.c',
  'resources.c',
  'settings.c',
  'subtitlehandler.c',
  'title-add.c',
  'titledict.c',
  'util.c',
  'values.c',
  'videohandler.c',
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

ui = gnome.compile_resources('ui_res', 'ui/ui_res.gresource.xml',
  source_dir: 'ui',
  c_name: 'ghb_ui')

# Build targets
executable('ghb',
  sources: [ghb_src, data, icons, ui],
  dependencies: ghb_deps,
  include_directories: hb_incdirs,
  link_language: 'cpp',
  link_args: get_option('c_link_args'),
  install: true)
