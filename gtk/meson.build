# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (c) 2023 HandBrake Team

project('handbrake-gtk', 'c', 'cpp',
  version: run_command('data/version.sh', check: true).stdout().strip(),
  license: 'GPL-2.0-only',
  meson_version: '>= 0.51.0',
  default_options: ['warning_level=1', 'buildtype=release'])

# Minimum versions for dependencies
glib_min = '>= 2.56'
gtk4_min = '>= 4.4'
gtk3_min = '>= 3.22'
gst_min  = '>= 1.10'

cc = meson.get_compiler('c')
i18n = import('i18n')
gnome = import('gnome')

hb_dir = get_option('hb-dir')
if hb_dir == ''
  hb_dir = meson.current_build_dir() / '..'
endif

hb_libdirs = [hb_dir / 'libhb', hb_dir / 'contrib/lib'] + get_option('extra-libdirs')
hb_incdirs = include_directories(hb_dir / 'libhb', hb_dir / 'contrib/include')

# External dependencies (required)
ghb_deps = [
  cc.find_library('handbrake', dirs: hb_libdirs),
  cc.find_library('bz2', dirs: hb_libdirs),
  cc.find_library('m', required: false),
  cc.find_library('mp3lame', dirs: hb_libdirs),
  dependency('dvdnav'),
  dependency('dvdread'),
  dependency('dav1d'),
  dependency('SvtAv1Enc'),
  dependency('fribidi'),
  dependency('glib-2.0', version: glib_min),
  dependency('gio-2.0', version: glib_min),
  dependency('gthread-2.0', version: glib_min),
  dependency('gmodule-2.0', version: glib_min),
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

if get_option('gtk4')
  ghb_deps += dependency('gtk4', version: gtk4_min)
else
  ghb_deps += dependency('gtk+-3.0', version: gtk3_min)
endif

if get_option('gstreamer').enabled()
  gst = dependency('gstreamer-1.0', version: gst_min, required: false)
  gst_audio = dependency('gstreamer-audio-1.0', required: false)
  gst_video = dependency('gstreamer-video-1.0', required: false)
  gst_pbutils = dependency('gstreamer-pbutils-1.0', required: false)
  if gst_pbutils.found()
    ghb_deps += [gst, gst_audio, gst_video, gst_pbutils]
    add_project_arguments('-D_ENABLE_GST', language: 'c')
  else
    message('Preview support disabled as gstreamer-plugins-base was not found.')
  endif
endif

if get_option('libdovi').enabled()
  ghb_deps += dependency('dovi')
endif

if get_option('fdk-aac').enabled()
  ghb_deps += dependency('fdk-aac')
endif

if get_option('qsv').enabled()
  if host_machine.system() == 'windows'
    ghb_deps += cc.find_library('vpl', dirs: hb_libdirs)
  else
    ghb_deps += [dependency('vpl'), dependency('libva'), dependency('libva-drm')]
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
  ghb_deps += cc.find_library('iconv', dirs: get_option('extra-libdirs'))
  add_project_arguments('-DLIBICONV_PLUG', language: ['c', 'cpp'])
endif

if not get_option('c_std').contains('gnu')
  add_project_arguments('-D_DEFAULT_SOURCE', language: ['c', 'cpp'])
endif

link_args = cc.get_supported_link_arguments(
  '-Wl,--export-dynamic', '-Wl,--exclude-libs,ALL')

add_project_link_arguments(link_args, language: ['c', 'cpp'])

subdir('po')
subdir('icons')
subdir('data')
subdir('src')
