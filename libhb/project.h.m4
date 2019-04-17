changequote(<<, >>)dnl
include(<<handbrake.m4>>)dnl
dnl
dnl
dnl
#ifndef HB_PROJECT_H
#define HB_PROJECT_H

<<#>>define HB_PROJECT_TITLE                     "__HB_title"
<<#>>define HB_PROJECT_NAME                      "__HB_name"
<<#>>define HB_PROJECT_NAME_LOWER                "__HB_name_lower"
<<#>>define HB_PROJECT_NAME_UPPER                "__HB_name_upper"
<<#>>define HB_PROJECT_URL_WEBSITE               "__HB_url_website"
<<#>>define HB_PROJECT_URL_COMMUNITY             "__HB_url_community"
<<#>>define HB_PROJECT_URL_IRC                   "__HB_url_irc"
<<#>>define HB_PROJECT_URL_APPCAST               "__HB_url_appcast"
<<#>>define HB_PROJECT_VERSION_MAJOR             __HB_version_major
<<#>>define HB_PROJECT_VERSION_MINOR             __HB_version_minor
<<#>>define HB_PROJECT_VERSION_POINT             __HB_version_point
<<#>>define HB_PROJECT_VERSION                   "__HB_version"
<<#>>define HB_PROJECT_VERSION_HEX               0x<<>>__HB_version_hex<<>>LL
<<#>>define HB_PROJECT_BUILD                     __HB_build
<<#>>define HB_PROJECT_REPO_URL                  "__HB_repo_url"
<<#>>define HB_PROJECT_REPO_TAG                  "__HB_repo_tag"
<<#>>define HB_PROJECT_REPO_REV                  __HB_repo_rev
<<#>>define HB_PROJECT_REPO_HASH                 "__HB_repo_hash"
<<#>>define HB_PROJECT_REPO_BRANCH               "__HB_repo_branch"
<<#>>define HB_PROJECT_REPO_REMOTE               "__HB_repo_remote"
<<#>>define HB_PROJECT_REPO_TYPE                 "__HB_repo_type"
<<#>>define HB_PROJECT_REPO_OFFICIAL             __HB_repo_official
<<#>>define HB_PROJECT_REPO_DATE                 "__HB_repo_date"

<<#>>define HB_PROJECT_HOST_SPEC                 "__HOST_spec"
<<#>>define HB_PROJECT_HOST_MACHINE              "__HOST_machine"
<<#>>define HB_PROJECT_HOST_VENDOR               "__HOST_vendor"
<<#>>define HB_PROJECT_HOST_SYSTEM               "__HOST_system"
<<#>>define HB_PROJECT_HOST_SYSTEMF              "__HOST_systemf"
<<#>>define HB_PROJECT_HOST_RELEASE              "__HOST_release"
<<#>>define HB_PROJECT_HOST_TITLE                "__HOST_title"
<<#>>define HB_PROJECT_HOST_ARCH                 "__HOST_arch"

<<#>>define HB_PROJECT_FEATURE_asm               __FEATURE_asm
<<#>>define HB_PROJECT_FEATURE_fdk_aac           __FEATURE_fdk_aac
<<#>>define HB_PROJECT_FEATURE_ffmpeg_aac        __FEATURE_ffmpeg_aac
<<#>>define HB_PROJECT_FEATURE_flatpak           __FEATURE_flatpak
<<#>>define HB_PROJECT_FEATURE_gtk               __FEATURE_gtk
<<#>>define HB_PROJECT_FEATURE_gtk_mingw         __FEATURE_gtk_mingw
<<#>>define HB_PROJECT_FEATURE_gtk_update_checks __FEATURE_gtk_update_checks
<<#>>define HB_PROJECT_FEATURE_gst               __FEATURE_gst
<<#>>define HB_PROJECT_FEATURE_nvenc             __FEATURE_nvenc
<<#>>define HB_PROJECT_FEATURE_qsv               __FEATURE_qsv
<<#>>define HB_PROJECT_FEATURE_vce               __FEATURE_vce
<<#>>define HB_PROJECT_FEATURE_x265              __FEATURE_x265
<<#>>define HB_PROJECT_FEATURE_numa              __FEATURE_numa

#endif /* HB_PROJECT_PROJECT_H */
