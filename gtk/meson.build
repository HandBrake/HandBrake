# Copyright (C) 2023-2024 HandBrake Team
# SPDX-License-Identifier: GPL-2.0-or-later

project('handbrake-gtk', 'c', 'cpp',
  version: run_command('data/version.py', check: true).stdout().strip(),
  license: 'GPL-2.0-only',
  meson_version: '>= 0.51.0',
  default_options: ['warning_level=2', 'buildtype=release'])

# Minimum versions for dependencies
glib_min = '>= 2.68'
gtk_min  = '>= 4.4'

cc = meson.get_compiler('c')
i18n = import('i18n')
gnome = import('gnome')

hb_dir = get_option('hb-dir')
if hb_dir == ''
  hb_dir = meson.current_build_dir() / '..'
endif

hb_libdirs = [hb_dir / 'libhb', hb_dir / 'contrib/lib']
hb_incdirs = include_directories(hb_dir / 'libhb', hb_dir / 'contrib/include')

if host_machine.system() == 'windows'
  win_libdirs = { 'dirs': hb_libdirs }
else
  win_libdirs = {}
endif

# External dependencies (required)
ghb_deps = [
  cc.find_library('handbrake', dirs: hb_libdirs),
  cc.find_library('bz2', kwargs: win_libdirs),
  cc.find_library('m'),
  cc.find_library('mp3lame', kwargs: win_libdirs),
  dependency('dvdnav'),
  dependency('dvdread'),
  dependency('dav1d'),
  dependency('SvtAv1Enc'),
  dependency('fribidi'),
  dependency('glib-2.0', version: glib_min),
  dependency('gio-2.0', version: glib_min),
  dependency('gthread-2.0', version: glib_min),
  dependency('gmodule-2.0', version: glib_min),
  dependency('gtk4', version: gtk_min),
  dependency('jansson'),
  dependency('libass'),
  dependency('libavcodec'),
  dependency('libavfilter'),
  dependency('libavformat'),
  dependency('libavutil'),
  dependency('libbluray'),
  dependency('liblzma'),
  dependency('libpostproc'),
  dependency('libswresample'),
  dependency('libswscale'),
  dependency('libxml-2.0'),
  dependency('ogg'),
  dependency('opus'),
  dependency('speex'),
  dependency('theoradec'),
  dependency('theoraenc'),
  dependency('threads'),
  dependency('libturbojpeg'),
  dependency('vorbis'),
  dependency('vorbisenc'),
  dependency('vpx'),
  dependency('x264'),
  dependency('zimg'),
  dependency('zlib'),
]

if get_option('libdovi').enabled()
  ghb_deps += dependency('dovi')
endif

if get_option('fdk-aac').enabled()
  ghb_deps += dependency('fdk-aac')
endif

if get_option('qsv').enabled()
  ghb_deps += dependency('vpl')
  if host_machine.system() != 'windows'
    ghb_deps += [dependency('libva'), dependency('libva-drm'), dependency('libdrm')]
  endif
endif

if get_option('x265').enabled()
  ghb_deps += dependency('x265')
  if get_option('numa').enabled()
    numa = dependency('numa', required: false)
    if not numa.found()
      numa = cc.find_library('numa')
    endif
    ghb_deps += numa
  endif
endif

if get_option('mf').enabled()
  ghb_deps += [cc.find_library('mfplat'), cc.find_library('strmiids')]
endif

# OS-specific dependencies

if host_machine.system() == 'windows'
  add_project_link_arguments('-mwindows', language: ['c', 'cpp'])
  win_dlls = ['bcrypt', 'ntdll', 'ole32', 'regex', 'userenv', 'uuid', 'ws2_32']
  foreach dll: win_dlls
    ghb_deps += cc.find_library(dll)
  endforeach
elif not cc.has_function('dlopen')
  ghb_deps += cc.find_library('dl')
endif

if not cc.has_function('iconv')
  ghb_deps += cc.find_library('iconv')
  add_project_arguments('-DLIBICONV_PLUG', language: ['c', 'cpp'])
endif

if not get_option('c_std').contains('gnu')
  add_project_arguments('-D_DEFAULT_SOURCE', language: ['c', 'cpp'])
endif

if (cc.has_function('strerror_r'))
  add_project_arguments('-DHAS_STRERROR_R', language: 'c')
endif

add_project_arguments('-DGDK_VERSION_MIN_REQUIRED=GDK_VERSION_4_4',
'-DGDK_VERSION_MAX_ALLOWED=GDK_VERSION_4_4', language: 'c')

compile_args = cc.get_supported_arguments(
  '-Wno-missing-field-initializers',
  '-Wno-sign-compare',
  '-Wno-unused-parameter',
  '-Wignored-qualifiers',
  '-Wimplicit-function-declaration',
  '-Wincompatible-pointer-types',
  '-Wint-conversion',
  '-Wint-to-pointer-cast',
  '-Wlogical-op',
  '-Wmissing-format-attribute',
  '-Wmissing-noreturn',
  '-Wmissing-include-dirs',
  '-Wnested-externs',
  '-Wold-style-definition',
  '-Woverflow',
  '-Wpointer-to-int-cast',
  '-Wshadow',
  '-Wstrict-prototypes',
  '-Wunused',
  '-Wwrite-strings')

add_project_arguments(compile_args, language: 'c')

link_args = cc.get_supported_link_arguments(
  '-Wl,--export-dynamic', '-Wl,--exclude-libs,ALL')
add_project_link_arguments(link_args, language: ['c', 'cpp'])

subdir('po')
subdir('icons')
subdir('data')
subdir('src')
