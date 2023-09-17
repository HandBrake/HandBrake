# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (c) 2023 HandBrake Team

widget_deps = custom_target('widget.deps',
  output: ['widget.deps', 'widget_reverse.deps'],
  command: find_program('makedeps.py'))

data = gnome.compile_resources('data_res', 'data_res.gresource.xml',
  dependencies: widget_deps,
  source_dir: meson.current_build_dir(),
  extra_args: '--manual-register',
  c_name: 'ghb_data')

marshalers = gnome.genmarshal('marshalers',
  sources: 'marshalers.list',
  prefix: 'ghb_marshal',
  extra_args: '--quiet')

i18n.merge_file(
  input: 'fr.handbrake.ghb.desktop.in',
  output: 'fr.handbrake.ghb.desktop',
  install: host_machine.system() not in ['windows', 'haiku'],
  install_dir: get_option('datadir') / 'applications',
  po_dir: po_dir,
  type: 'desktop')

i18n.merge_file(
  input: 'fr.handbrake.ghb.metainfo.xml.in',
  output: 'fr.handbrake.ghb.metainfo.xml',
  install: host_machine.system() not in ['windows', 'haiku'],
  install_dir: get_option('datadir') / 'metainfo',
  po_dir: po_dir,
  type: 'xml')

if host_machine.system() not in ['windows', 'haiku']
  meson.add_install_script('post_install.py',
    get_option('prefix') / get_option('datadir'))
endif
