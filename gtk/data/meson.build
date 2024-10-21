# Copyright (C) 2023-2024 HandBrake Team
# SPDX-License-Identifier: GPL-2.0-or-later

data = gnome.compile_resources('data_res', 'data_res.gresource.xml',
  source_dir: meson.current_build_dir(),
  c_name: 'ghb_data')

desktop = i18n.merge_file(
  input: 'fr.handbrake.ghb.desktop.in',
  output: 'fr.handbrake.ghb.desktop',
  install: host_machine.system() not in ['windows', 'haiku'],
  install_dir: get_option('datadir') / 'applications',
  po_dir: po_dir,
  type: 'desktop')

metainfo_conf = configuration_data()
metainfo_conf.set('RELEASE_TAG', meson.project_version())
metainfo_conf.set('RELEASE_DATE', run_command('version.py', 'date', check: true).stdout().strip())

metainfo_in = configure_file(
  input: 'fr.handbrake.ghb.metainfo.xml.in.in',
  output: 'fr.handbrake.ghb.metainfo.xml.in',
  configuration: metainfo_conf)

metainfo = i18n.merge_file(
  input: metainfo_in,
  output: 'fr.handbrake.ghb.metainfo.xml',
  install: host_machine.system() not in ['windows', 'haiku'],
  install_dir: get_option('datadir') / 'metainfo',
  po_dir: po_dir,
  type: 'xml')

appstreamcli = find_program('appstreamcli', required: false, disabler: true)
test('Validate metainfo file', appstreamcli,
  args: ['validate', '--strict', '--no-net', '--explain', metainfo])

desktop_utils = find_program('desktop-file-validate', required: false, disabler: true)
test('Validate desktop file', desktop_utils, args: desktop)

if host_machine.system() not in ['windows', 'haiku']
  meson.add_install_script('post_install.py',
    get_option('prefix') / get_option('datadir'))
endif
