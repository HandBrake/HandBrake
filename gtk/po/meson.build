# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (c) 2023-2025 HandBrake Team

po_dir = meson.current_source_dir()

if host_machine.system() == 'netbsd'
  meson.override_find_program('msgfmt', find_program('/usr/pkg/bin/msgfmt'))
elif host_machine.system() == 'sunos'
  meson.override_find_program('msgfmt', find_program('gmsgfmt'))
endif

i18n.gettext('ghb',
  preset: 'glib',
  args: ['--package-version=' + meson.project_version(),
         '--copyright-holder=HandBrake Team'])
