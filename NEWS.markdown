# HandBrake News


## Upgrade Notice

Before updating HandBrake, please make sure there are no pending encodes in the queue, and be sure to make a backup of any custom presets and app preferences you have, as they may not be compatible with newer versions.

Windows users, please make sure to install [Microsoft .NET Desktop Runtime version 8.0](https://dotnet.microsoft.com/en-us/download/dotnet/8.0/runtime)
Download available from Microsoft:
- [For x64 (AMD or Intel CPUs)](https://aka.ms/dotnet/8.0/windowsdesktop-runtime-win-x64.exe)
- [For Arm64 (Qualcomm or other)](https://aka.ms/dotnet/8.0/windowsdesktop-runtime-win-arm64.exe)

## HandBrake 1.9.0

#### General
- Added Intel QSV VVC (hardware) video decoder

### Windows
- Add Range Limit controls to the "Add to Queue" selection window. (#4146)
- Add support for DirectX based video decoding when using the Media Foundation encoder on ARM devices. 
- Miscellaneous bug fixes and improvements


## HandBrake 1.8.2

### All platforms

#### General

- Fixed a potential crash when trying to scan deleted files
- Fixed scan of broken video that uses reserved color matrix values
- Fixed an issue that could cause some audio tracks to be dropped

#### Third-party libraries

- Updated libraries
  - FFmpeg 7.0.2 (decoding and filters)
  - libass 0.17.3 (subtitles)
  - libvpx 1.14.1 (VP8/VP9 video encoding)

### Windows
- Fixed an issue where auto name wasn't triggering correctly with preset changes (#6159)
- Fixed a potential crash when importing presets from the mac version. (#6186)
- Fixed an issue loading presets where a video encode isn't available on the system. (#6184)
- Minor startup performance improvement on some Intel based systems.

### Linux
- Fixed an issue where an encoded file could be output to the wrong filename when using the queue (#6067)


## HandBrake 1.8.1

### All platforms

#### Video

- Fixed a warning / misconfiguration when repeat-headers is used with the x265 encoder.(#6061)
- Fixed an issue where the NVEnc encoder ignored the level option

#### Subtitles

- Fixed an issue where dvd subtitles could be corrupted during rendering.

#### Filters

- Fixed an issue that caused video artefacts to occur when using the eedi2 filter (#6073)

#### Third-party libraries

- Updated libraries
  - FFmpeg 7.0.1 (decoding and filters)
  - libdav1d 1.4.3 (AV1 video decoding)

### Mac

- Fixed a stall in the queue that could happen if the encoding process crashes when configuring a job
- Removed the "Show" button from the notifications when there is nothing to show

### Windows

- Hardware decoding is now defaulted to off for new installations. Users can opt-in whilst making sure they are running up-to-date drivers
- Fixed an issue where hardware decoding could erroneously be used for previews
- Fixed an issue where auto name was triggering too aggressively (#6079)
- Removed an erroneous error message when dragging files onto the main window that include a subtitle file.(#6065)


## HandBrake 1.8.0

### All platforms

#### General

- Allowed muxing VP9 and FLAC in the MP4 container
- Removed timestamp jitters when using a constant NTSC frame rate in the MP4 container
- Removed support for importing legacy plist based presets from much older versions of HandBrake
- Updated iso639 language codes list

#### Video

- Added support for the FFV1 encoder, including a new preset "Preservation FFV1" under the Professional category
- Added support for multi-pass CQ with VP9
- Added support for VP9 tunes
- Added Dolby Vision dynamic metadata pass through for SVT-AV1
- Improved Decomb speed by removing unneeded frame copies
- Improved Framerate Shaper metrics for high depth frames

#### Command line interface

- Fixed subtitle "scan" when not first in --subtitle list
- Fixed override of subtitle settings
- Fixed processing audio overrides to preset

#### Audio

- Added TrueHD encoder
- Added 88.2/96/176.4/192 kHz sample rates for TrueHD and FLAC encoders
- Improved audio tracks selection by tracking "linked" audio tracks
- Fixed incorrect channel layout when encoding a 6.1 track to Opus

#### Subtitles

- Fixed passthru of VobSub tracks that contains empty of fully transparent subtitles samples
- Fixed an issue that prevented decoding VobSub tracks stored inside MP4
- Fixed burn-in of SSA/ASS subtitles inside MKV that have duplicated Read Orders

#### Build system

- Reviewed and improved compiler optimization options for the third-party libraries
- Improved libdovi package to make it possible to ship it in Flathub flatpak version

#### Third-party libraries

- Updated libraries
  - AMF 1.4.33 (AMD VCN video encoding)
  - FFmpeg 7.0 (decoding and filters)
  - HarfBuzz 8.4.0 (subtitles)
  - libass 0.17.2 (subtitles)
  - libdav1d 1.4.1 (AV1 video decoding)
  - libdovi 3.3.0 (Dolby Vision dynamic metadata)
  - libopus 1.5.2 (Opus audio encoding)
  - libjpeg-turbo 3.0.3 (preview image compression)
  - libvpx 1.14.0 (VP8/VP9 video encoding)
  - oneVPL 2.10.1 (Intel QSV video encoding/decoding)
  - SVT-AV1 2.1.0 (AV1 video encoding)
  - x264 164 r3186 (H.264/AVC video encoding)
  - x265 3.6 (H.265/HEVC video encoding)
  - zlib 1.3.1 (general)
- Removed libraries
  - libxml2
  
### Linux

- Migrated the UI to GTK 4
- Added support for resursive file scan
- Added support for drag/drop/multi-file scanning. (Including subtitle files)
- Added --clear-queue and --auto-start-queue flags to the linux ui app.
- Update the "Open Source" button to make the folder/batch mode more discoverable. 
- Refreshed app icons
- Miscellaneous bug fixes and improvements
- Updated existing translations

### Mac

- Added Dolby Vision and HDR10+ dynamic metadata pass through for VideoToolbox H.265 10-bit
- Added Metal accelerated Comb Detect and Framerate Shaper filters
- Added ability to drag and drop SRT and ASS subtitles files onto the main window
- Added support for excluding file extensions when opening files in batch mode
  - Default exclusions are common image, subtitles, and text file extensions; edit list in Preferences > Advanced
- Added new preference to pause encodes when switching to battery
- Added new preference to clear completed jobs at launch option
- Reworked notification options to allow per encode or per queue control
- Reworked toolbar to follow macOS UI style
- Improved performance of the Activity Log window
- Improved UI performances by avoiding uneeded window resizes and slow AppKit controls
- Miscellaneous bug fixes and improvements
- Updated existing translations
- Added new translations
  - Ukrainian

### Windows

- Requires Microsoft .NET Desktop Runtime 8.0.x
- Added Invert Queue option to the Add to Queue Window (#5741)
- Drag/Drop now supports resursive folder scan mode.
- Miscellaneous bug fixes and improvements
- Updated existing translations
- Added new translations
  - Catalan (Català)


## HandBrake 1.7.3

### All platforms

#### General

- Fixed formatting leading zeros for timestamps in logs
- Miscellaneous bug fixes and improvements

#### Video

- Fixed an issue that could potentially cause incorrect detection of source FPS value (#5677)

#### Subtitles

- Fixed tx3g to SSA subtitles conversion

### Linux

- Fixed the "All Files" open file dialog filter to really show all files
- Fixed an issue where adding a new default audio track automatically set the gain to -20 dB

### macOS

- Fixed a regression introduced in HandBrake 1.7.2 by the workaround for VideoToolbox crashes on Sonoma
- Miscellaneous bug fixes and improvements

### Windows

- Improved security hardening of loading of DLL files within libhb by limiting search scope (#5724)
- Improved keyboard navigation on Chapters tab (#5679)
- Fixed an issue that could cause encodes to appear stalled when process isolation is turned on
- Fixed an issue that could prevent QSV decode from being used when a system also has a non-Intel GPU
- Fixed an issue where the drive label was not always correctly detected (#5711)
- Miscellaneous bug fixes and improvements


## HandBrake 1.7.2

### All platforms

#### Video

- Improved automatic selection of Dolby Vision Level
- Fixed an issue in AMD VCN, MPEG-2, MPEG-4, NVIDIA NVENC, and VP9 encoders that could cause the creation of an excessive number of keyframes (#5530)
- Fixed unintentional automatic pass through of closed caption side data when using NVIDIA NVENC encoder
- Miscellaneous bug fixes and improvements

### Linux

- Improved performance by removing duplicate graphic assets and reducing file size
- Fixed Flatpak file chooser opening home directory instead of the previously selected directory
- Fixed last item in the queue sometimes having the wrong destination path
- Fixed some memory leaks in the graphical interface

### Mac

- Added a workaround to prevent a system crash when using VideoToolbox encoders with macOS 14 Sonoma on Apple Silicon Ultra
- Added a workaround to prevent issues decoding H.264 video by disabling VideoToolbox hardware decoding for Level 6.1 and 6.2
- Fixed a crash opening an empty folder

### Windows

- Added Automation Properties to some controls on the audio tab to assist screen readers
- Fixed NVIDIA NVDEC option being ignored (#5569)
- Fixed startup crash related to theme loading (#5567)
- Fixed subtitle language order not being honoured (#5590)


## HandBrake 1.7.1

### All platforms

#### Video

- Fixed Decomb filter producing corrupted frames with 10-bit depth or higher source (#5518)
- Fixed a potential crash when using the Comb Detect filter on an RGB source
- Fixed Intel QSV devices enumeration where devices from other vendors are present (#5317, #5177)

#### Mac

- Fixed an issue opening EyeTV sources (#5514)


## HandBrake 1.7.0

### All platforms

#### General

- Added Apple VideoToolbox hardware presets
- Updated Creator presets
  - Disabled interlacing detection and removal; assume creators are working with progressive sources by default
- Updated Social presets
  - Target higher quality and frame rate over shorter durations, without interlacing detection and removal
  - Better suited for modern social sharing of short live action clips and screen/game captures
- Removed Email presets in favor of revised Social presets
  - Please stop sending videos via email or use the new Social presets
- Miscellaneous bug fixes and improvements

#### Video

- Added AMD VCN AV1 encoder
- Added NVIDIA NVENC AV1 encoder
- Added support for SVT-AV1 multi-pass ABR mode
- Added support for preserving ambient viewing enviroment metadata
- Added QSV Rotate and Format filters
- Improved performance on arm64 / aarch64 / Apple Silicon architectures
  - Latest FFmpeg provides faster HEVC decoding, 30% faster bwdif filter
  - New SVT-AV1 assembly optimizations provide up to 4x increase in performance
- Improved video conversion speed by removing unneeded frame copies for better memory efficiency
- Improved Dolby Vision dynamic range metadata pass through
  - Supported encoders: x265 10-bit
  - Supported profiles and cross-compatibility IDs: 8.4, 8.1, 7.6 (base layer only, converted to 8.1), 5.0
- Improved HDR10+ dynamic range metadata pass through
  - Supported encoders: x265 10-bit, SVT-AV1
- Improved QSV support on Linux (#4958)
- Updated NVENC to not use multi-pass by default; user configurable advanced option
- Renamed 2-pass encode option to multi-pass (#5019)
- Fixed Intel QSV encoder outputting green video in some cases (#4842, #4876)
- Fixed pixel format conversion slightly altering colors when using a 10-bit hardware encoder (#5011)
- Fixed scan failures by using swscale instead of zscale when source resolution is not mod 2
- Fixed incorrect PAR when reading from an anamorphic AV1 video track
- Removed an artificial bitrate limit on VP9 CQ mode

#### Command line interface

- Renamed `--two-pass` to `--multi-pass` and `--no-two-pass` to `--no-multi-pass`, removed `-2` (#5019)
- Fixed automatic cropping enabled despite using preset with cropping disabled (#5055)

#### Audio

- Fixed low volume level when downmixing ac3 and eac3
- Fixed left-only and right-only mono mixdowns (#3533, #5054)

#### Subtitles

- Fixed locale settings potentially causing incorrect decimal separator in SSA headers
- Fixed a potential issue affecting zero-duration subtitles

#### Build system

- Added Meson build system for the Linux GUI

#### Third-party libraries

- New libraries
  - libdovi 3.2.0 (Dolby Vision dynamic range metadata)
- Updated libraries
  - AMF 1.4.30 (AMD VCN video encoding)
  - FFmpeg 6.1 (decoding and filters)
  - FreeType 2.13.2 (subtitles)
  - Fribidi 1.0.13 (subtitles)
  - HarfBuzz 8.2.2 (subtitles)
  - libass 0.17.1 (subtitles)
  - libdav1d 1.3.0 (AV1 video decoding)
  - liblzma (xz) 5.4.5 (LZMA video decoding, e.g. TIFF)
  - libopus 1.4 (Opus audio encoding)
  - libjpeg-turbo 3.0.1 (preview image compression)
  - libvpx 1.13.1 (VP8/VP9 video encoding)
  - libxml 2.11.5 (general)
  - oneVPL 2023.3.1 (Intel QSV video encoding/decoding)
  - SVT-AV1 1.7 (AV1 video encoding)
  - x264 164 r3107 (H.264/AVC video encoding)
  - x265 r12776 (H.265/HEVC video encoding)
  - zimg 3.0.5 (color conversion)
  - zlib 1.3 (general)

### Linux

- Added drag and drop support for video scanning
- Added support for native file choosers via xdg-desktop-portal
- Added Queue > Add All menu option
- Added XML chapter import and export
- Added bit depth and HDR information to video summary
- Added option to pause encoding when switching to battery power or when power save mode is activated
- Added automatic file naming options: {codec} {bit-depth} {width} {height} {modification-date} {modification-time}
- Updated Queue, Activity, and Presets windows to no longer float on top of the main window
- Updated existing translations
- Removed obsolete update checker
- Miscellaneous bug fixes and improvements

### Mac

- Added support for drag and drop of multiple files at once
- Added support for selecting multiple files at once in the Open Source dialog
- Added support for recursive folder scanning in the Open Source dialog
- Added support for VideoToolbox H.265/HEVC, H.264/AVC, ProRes, and VP9 hardware decoders on macOS 13 and later
  - Enable/disable in the Preferences > Advanced tab always or full path only
  - Using hardware decoders on modern devices may decrease CPU usage and thus speed up some filters and encoding
- Added GPU accelerated Crop & Scale, Rotate, Pad, Yadif, Bwdif, Chroma Smooth, Unsharp, Lasharp, Grayscale filters
- Added "Same as source" destination option that automatically sets the destination path to the source path
- Improved SVT-AV1 encoding performance by up to 4x on Apple Silicon Macs
- Improved automatic file naming Preferences UI and added new options: {width} {height} {quality_type} {encoder_bit_depth} {modification-time} {modification-date} {codec} {encoder} {encoder_bit_depth} {preset}
- Improved handling of security scoped bookmarks
- Fixed Chroma Smooth tune options
- Fixed Deblock Filter custom string field
- Fixed an issue that prevented the VideoToolbox "speed" preset from being used
- Fixed the file size display on the queue statistics window when file size info is not available
- Miscellaneous bug fixes and improvements
- Added new translations
  - Korean (한국어)
  - Bulgarian (Български)
- Updated existing translations

### Windows

- Added support for drag and drop of multiple files at once
- Added support for selecting multiple files at once in the Open Source dialog
- Added support for recursive folder scanning in the Open Source dialog
  - Enable/disable in Preferences > Advanced
- Added support for excluding file extensions when opening files in batch mode
  - Default exclusions are common image, subtitles, and text file extensions; edit list in Preferences > Advanced
- Improved Preview window native video playback to support most containers and codecs
  - Modern codec support requires Microsoft Codec Packs from the Microsoft Store
- Improved automatic file naming Preferences UI and added new options: {width} {height} {quality_type} {encoder_bit_depth} {modification-time} {modification-date} {encoder} {encoder_bit_depth} {preset}
- Improved Queue window UI to optionally show additional status information on the left progress panel
- Improved Presets panel
  - Manage Presets button replaced with a menu of discrete options for quicker access to functionality
  - Added an option to display the description for the selected preset
  - Added the ability to clone a preset (create a new preset based on an existing one)
- Improved Add Selection window to make sorting feature more discoverable
- Improved Process Isolation reliability
- Fixed automatic file naming when using physical drive sources (#4859)
- Fixed Title Specific Scan for physical drive sources (#4921)
- Fixed an potential issue that could cause an unmodified preset to display as "modified" (#4909, #4908)
- Fixed an potential issue where changes to queue order were not retained (#4922)
- Fixed an Audio tab issue where using a non-fallback encoder could lead to duplicated tracks (#5012)
- Fixed an issue where swapping graphics cards might cause hardware presets to be incorrectly shown as disabled
- Fixed a potential crash on startup related to Windows Notifications Service failures (#5097)
- Miscellaneous bug fixes and improvements
- Added new translations
 - Czech (česky) (partially complete)
 - Greek (Ελληνικά) (partially complete)
 - Estonian (Eesti) (partially complete)
 - Basque (Euskara)
 - Finnish (Suomi) (partially complete)
- Updated existing translations


## HandBrake 1.6.1

### All platforms

#### Video

- Fixed a potential decoder issue that could cause desync with audio (#4788, #4789)

#### Command line interface

- Fixed inability to name external subtitles tracks using --subname

### Mac

- Fixed behavior of quality slider when changing encoders

### Linux

- Fixed translations missing updates as part of 1.6.0 (#4790)
  - Bulgarian (Български)
  - Corsican (Corsu)
  - Dutch (Nederlands)
  - German (Deutsch)
  - Spanish (Español)
- Fixed (partially) Intel QSV hardware detection (#4768)
- Fixed a potential crash when canceling an Intel QSV encode (#4341)
- Fixed building with -Werror=format-security by adding missing format strings where needed

### Windows

- Fixed quality slider not allowing negative values for encoders supporting them
- Fixed issues upgrading presets from older versions (#4820)
- Fixed a potential graphical interface hang when stopping the queue (#4782)
- Fixed optical disc drives on the source selection pane not scanning correctly (#4771)
- Fixed erroneous display of 2-pass check box for Intel QSV AV1 encoder (not yet supported) (#4777)
- Fixed a build configuration issue that broke version 1.6.0 for Windows on arm64
- Fixed an issue that prevented NVDEC from being available
- Fixed passthru audio erroneously falling back to encoding (#4795)
- Fixed the Save New Preset button incorrectly overwriting recently added presets (#4804)


## HandBrake 1.6.0

### All platforms

#### General

- Added AV1 video encoding
- Added high bit depth and color depth support to various encoders and filters
- Added 4K AV1 General, QSV (Hardware), and MKV (Matroska) presets
- Added 4K HEVC General presets and updated related presets to use similar encoder settings
- Revised Web presets and renamed to Creator, Email, and Social
- Removed VP8 presets
  - The VP8 video encoder is now deprecated and will be removed in a future release
  - Related, the Theora encoder is long deprecated and will be removed in a future release
- Miscellaneous other preset revisions

#### Video

- Added SVT-AV1 (software) and Intel QSV AV1 (hardware) video encoders
- Added VP9 10-bit encoder
- Added NVENC HEVC 10-bit encoder
- Added VCN HEVC 10-bit encoder
- Added H.264 levels 6, 6.1, and 6.2 for the x264 encoder
- Added H.264/H.265 4:2:2 and 4:4:4 profiles for the x264 and x265 encoders
- Added H.265 4:2:2 profile for VideoToolbox encoder on Apple Silicon
- Added support for Intel Deep Link Hyper Encode (leverage multiple QSV media engines to increase performance)
- Fixed longstanding issue where slowest NVENC encoder preset caused encoding failures
- Removed support for Intel CPUs older than 6th generation (Skylake) when using Intel Quick Sync Video

#### Filters

- Added Bwdif deinterlace filter
- Improved Autocrop filter algorithm
  - Higher accuracy on mixed aspect ratio content, e.g. both letterbox and full frame pictures
- Improved video scaling performance on Apple Silicon and ARM-based systems
  - Use zscale (zimg) by default, falling back to swscale where necessary
- Improved multithread performance (slightly) for the following filters on high core count systems
  - Comb Detect
  - Decomb
  - Denoise
    - NLMeans
- Updated the following filters to support higher than 8-bit content and 4:2:2/4:4:4 chroma subsampling
  - Detelecine
  - Comb Detect
  - Decomb
  - Grayscale
  - Denoise
    - NLMeans
    - HQDN3D
  - Chroma Smooth
  - Sharpen
    - UnSharp
    - LapSharp

#### Build system

- Added support for building for OpenBSD
- Added --cpu configure parameter to enable building for native CPU architecture
- Added --lto configure parameter to enable link time optimization
- Miscellaneous bug fixes and improvements

#### Third-party libraries

- Updated libraries
  - AMF 1.4.24 (AMD VCN encoding)
  - FFmpeg 5.1.2 (decoding and filters)
  - FreeType 2.12.1 (subtitles)
  - Fribidi 1.0.12 (subtitles)
  - HarfBuzz 4.4.1 (subtitles)
  - libass 0.16.0 (subtitles)
  - libbluray 1.3.4 (Blu-ray decoding)
  - libdav1d 1.0.0 (AV1 decoding)
  - libdvdread 6.1.3 (DVD decoding)
  - liblzma (xz) 5.2.6 (LZMA video decoding, e.g. TIFF)
  - libjpeg-turbo 2.1.4 (preview image compression)
  - libspeex 1.2.1 (Speex audio decoding)
  - libvpx 1.12.0 (VP8/VP9 video encoding)
  - libxml 2.10.3 (general)
  - oneVPL 2023.1.0 (Intel QSV encoding/decoding)
  - x264 164 r3100 (H.264/AVC video encoding)
  - x265 r12747 (H.265/HEVC video encoding)
  - zimg 3.0.4 (color conversion)
- New libraries
  - SVT-AV1 1.4.1 (AV1 encoding)

### Linux

- Added many quality of life improvements contributed by community members
- Improved parity with the Mac and Windows graphical interfaces
- Miscellaneous bug fixes and improvements
- Updated translations
- Added new translations
  - Bulgarian (Български)
  - Finnish (Suomi)
  - Georgian (ქართული)

### Mac

- Added Quick Look support to the queue
- Miscellaneous bug fixes and improvements
- Updated translations
- Added Japanese translation

### Windows

- Miscellaneous bug fixes and improvements
- Updated all translations
- Added new translations
  - Bulgarian (Български)
  - Dutch (Nederlands)
  - Polish (Polski)


## HandBrake 1.5.1

### Linux

- Fixed an issue with the source tarball that broke Flathub builds (updated libass module to version 0.15.2)

### Windows

- The Windows UI is now .NET 6.0 only; .NET 5.0 is no longer additionally required


## HandBrake 1.5.0

### All platforms

#### Video

- Fixed an issue on older Intel CPUs causing the CLI to fail to initialize (#3924)
- Updated video engine to preserve chroma sample location information
- Updated Intel Quick Sync to use the Intel oneAPI Video Processing Library (oneVPL)

#### Audio

- Fixed MP2 audio sources not utilizing the fallback encoder when pass through is disabled (#3863)
- Fixed FFmpeg AAC audio encoder quality mode scale range (#1295)

#### Subtitles

- Fixed an issue with captions pass through durations (#3764)

#### Build system

- Fixed multiple potential race conditions in Flatpak build process
- Updated mac-toolchain-build script with newer tool versions

#### Third-party libraries

- Updated libraries
  - FFmpeg 4.4.1 (decoding and filters)
  - FreeType 2.11.1 (subtitles)
  - Fribidi 1.0.11 (subtitles)
  - HarfBuzz 3.1.2 (subtitles)
  - Jansson 2.14 (JSON architecture)
  - libass 0.15.2 (subtitles)
  - libdav1d 0.9.2 (AV1 decoding)
  - libjpeg-turbo 2.1.2 (preview image compression)
  - libogg 1.3.5 (Xiph codecs support)
  - libvpx 1.11.0 (VP8/VP9 video encoding)
  - zimg 3.0.3 (color conversion)

### Linux

- Fixed a filter settings issue that resulted in incorrect filters being added to jobs (#3910)
- Updated Intel QSV Flatpak plugin to use Intel MediaSDK 21.3.5
- Updated Flatpak dependencies
  - Freedesktop Platform 21.08
  - GNOME 41
- Added Corsican (Corsu) translation
- Updated translations
  - Simplified Chinese (简体中文)
  - Dutch (Nederlands)
  - French (Français)
  - Korean (한국어)
  - Spanish (Español)
  - Swedish (Svenska)
- Miscellaneous bug fixes and improvements

### Mac

- Requires macOS 10.13 High Sierra or later
- Fixed potential issues where encoding process could get stuck on efficiency cores in some cases
- Fixed an issue with automatic naming not applying correctly
- Fixed main window not being shown upon editing a job in the queue
- Updated queue statistics tab to include average encoding speed and percent of original file size
- Added "Move to top" and "Move to bottom" to the queue context menu
- Added a stop breakpoint to the queue, set when stop after job is selected and may be reordered (#2572)
- Added native support for the VideoToolbox API, enabling advanced features such as frame multi-pass, mastering display and content light metadata, and chroma location; the previous implementation via FFmpeg's libavcodec has been removed
- Added Corsican (Corsu) translation
- Updated translations
  - Simplified Chinese (简体中文)
  - French (Français)
  - German (Deutsch)
  - Italian (Italiano)
  - Portuguese (Português)
- Miscellaneous bug fixes and improvements

### Windows

- Requires Windows 10 or later and Microsoft .NET Desktop Runtime 6.0.0 or later
- Fixed a potential crash when encoding using Intel QSV in 2-pass mode (#4026)
- Fixed potential issues where encoding process could get stuck on efficiency cores in some cases
- Fixed an issue with static preview not updating when switching between automatic and custom cropping modes (#3911)
- Updated the toolbar preset dropdown to an overlay panel that utilizes a tree view which saves state
- Updated UI with various improvements to layout, typography, and cosmetics
- Updated queue statistics tab to include average encoding speed and content information
- Updated automatic naming file format placeholders to be live options
- Updated Send File To preference to set HB_SOURCE, HB_DESTINATION, and HB_EXIT_CODE environment variables, which can be used instead of arguments
- Added new exit_code parameter to Send File To preference
- Added "Move to top" and "Move to bottom" to the queue context menu
- Added a stop breakpoint to the queue, set when stop after job is selected and may be reordered (#2572)
- Added support for Windows 10/11 notifications, enable in Preferences > When Done
- Added support for Right to Left UI rendering for languages that require it. This can be set in preferences
- Added Traditional Chinese (正體中文) and Polish (Polski) translations
- Updated translations
  - Simplified Chinese (简体中文) - includes fix for access keys in menus
  - Corsican (Corsu)
  - German (Deutsch)
  - Italian (Italiano)
  - Japanese (日本語)
- Miscellaneous bug fixes and improvements


## HandBrake 1.4.2

### All platforms

#### General
- Improvements and fixes around colour bit-depth handling.
- Fixed various issues where incorrect colour information could be written during muxing.

#### Hardware Encoding
- Fixed corrupted video output when decoding HDR10 content with QuickSync

#### Subtitles
- Fixed a slight subtitle colour shift issue when using libass

### Mac
- Fixed a build system errors that could cause failures linking libbluray 
- Fixed incorrect documentation URL
- Fixed a possible crash that can occur when applying a malformed preset.

### Windows
- Added "Preset" used to the Queue summary tab.
- Added "Save New Preset" to the preset menu, and toolbar preset dropdown to make it easier to find. (#3783)
- Added category headers to the presets menu when using the flat display mode.
- Changed the log filename format back to start with the destination filename as it did with 1.3 (#3740)
- Changed tab selection behaviour on queue when changing jobs. it will no longer reset to the first tab. (#3813)
- Fixed a minor UI juddering effect on the queue task list when jobs start or finish (#3813)
- Fixed calculation errors when using padding on the dimensions tab (#3802)
- Fixed an issue where static preview wasn't live-updating when changes occurred (#3803)
- Fixed a crash on the Audio Defaults screen whilst trying to add Tracks (#3785)
- Fixed incorrect taskbar icon state when running multiple encodes (#3791)
- Fixed an issue where "None" resolution limit was not honoured (#3872)
- Fixed an issue with preset export. VideoTune was not written correctly (#3829)


## HandBrake 1.4.1

### All platforms

#### General
- Fixed a crash when using "Align AV" on Intel based systems. (#3683)
- Fixed a crash when reading certain DVD's with missing VOB files

#### Hardware Encoding
- Fixed an issue with QuickSync accelerated Crop/Scale generating incorrect aspect ratios (#3236)
- Fixed a crash after a subtitle scan when using the QuickSync encoder. (#3741)

#### Subtitles
- Backport some libass patches which should correct some issues with font and font-weight selections. (#3736)
- Fixed an issue that could prevent 3rd party software handling HandBrake files with dvb subtitles.

#### Command line interface
- Fixed a regression that prevented upscaling when using -w and -h (#3746)

### Linux
- Added: flatpak permission to show bookmarks in file dialogs (#3748)

### Mac
- Fix an issue where the Quality Slider was being ignored when using the VideoToolbox encoder. (#3751)
- Fixed an issue where incompatible hardware presets could be selected. 

### Windows
- Windows UI builds are now available for ARM64 devices. 
- Added upgrade notices to the installer welcome page advising of .NET Desktop Runtime 5 requirements and to complete existing queue. (#3693)
- Added support for software rendering which can be enabled to workaround issues with Variable Refresh Rate and 3rd party software causing rendering corruption (#3755)
- Added a new preference to define how the preset toolbar button renders the preset list. (#3697)
- Fixed a crash which would prevent all user settings from loading in some circumstances. 
- Fixed "Reset Settings" button in preferences. Certain settings were not reset correctly. (#3726)
- Fixed issues with the built-in updater that may cause it to fail to run the installer if the app was not running as admin.
- Fixed an issue that required and app restart to apply changes to the max simultaneous encodes setting. 
- Fixed an issue with Audio Defaults fallback encoder quality/bitrate/mixdown settings would not display correct values (#3739)
- Fixed an with QSV multi-instance support where multiple Intel GPU's are used.


## HandBrake 1.4.0

### All platforms

#### General
- The HandBrake engine is now 10 and 12bit capable. Please note that not all filters support 10 and 12 bits. Using an 8bit filter will cause the pipeline to run at 8bit. Please see the documentation for more information.
- HDR10 metadata will be passed through from the source file if present. 
- Static Previews that are generated during file scans are now stored in compressed jpeg format (previously stored as YUV420).  Temporary disk space usage and disk writes are massively reduced. This uses libjpeg-turbo

#### Filters
- New Filter: Chroma Smooth
- New Filter: Colourspace Selection. 
- New Filter: Support for QuickSync hardware accelerated Crop/Scale when using full path.

#### Hardware Encoding

- New Encoder: Media Foundation
  - For Windows based ARM64 devices powered by Qualcomm Chipsets. 
- Updates to the AMD VCN encoder:
  - Quality tuning for VCN's constrained vbr rate control mode. Results are the same or better than cqp mode, and bit rate is much more predictable.
  - Included optimised H265 presets for 1080p and 4K content.
- Updates to the Intel QuickSync encoder:
  - Minor performance improvement by skipping VFR and Crop/Scale filters when they are not required. 
  - Overhauled memory management including improved zero-copy support where software filters are not used which should also improve performance. 

#### Audio
- MP2 Audio Passthru support.

#### Subtitles
- New General purpose subtitle decoder
  - Added support for DVB Subtitles (Passthru and Burn-In)
  - Added support for EIA608 Closed Captions.
  - Replaced current decoders for PGS, SRT and SSA  with those in ffmpeg. This should correct a number of rendering issues on Burn-In
- Reduced default CC burn-in font-size.

#### Third-party libraries
- The following 3rd party libraries have changed:
  - ffmpeg 4.4
  - AMF 1.4.18 (AMD VCN encoding)
  - nv-codec-headers 11.0.10.1 (Nvidia NVENC encoding)
  - libmfx 1.34
  - freetype 2.10.4
  - fribidi 1.0.10
  - harfbuzz 2.8.1
  - jansson 2.13.1
  - libass 0.15.1
  - libbluray 1.3.0
  - libdvdnav 6.1.1
  - libdvdread 6.1.1
  - dav1d 0.9.0
  - libvorbis 1.3.7
  - libvpx 1.10.0
  - x264 161 r3043
  - x265 3.5
  - zimg and libjpeg-turbo are new dependencies. 

### General UI Updates (Applies to Windows, macOS and Linux)
- The "Dimensions" tab has been redesigned. 
  - The Rotate and Flip filter has been moved from the filters tab.
  - Added support for padding
  - Added support to control the resolution limit.
  - Added limited support for upscaling

### Linux
- Minor usability tweaks and fixes.
- Added new translations (levels of completeness vary):
  - Chinese (Taiwan)
  - Hebrew
  - Sinhala
  - Slovenian
- Updated existing translations (levels of completeness vary).

### Mac
- Support for Apple Silicon (macOS only)
- Support for running multiple simultaneous jobs.
- Support eyetv packages with .ts enclosed media file
- Improved UI navigation
  - Added two menu items to quickly switch between titles
  - Improved undo/redo support
  - Drag & drop import/export support in the presets popover
- Preference Updates: 
  - Added a preference to control whether the current edited preset should be re-applied when changing title
- Improved Security Scoped Bookmarks management
- Minor improvements and fixes for macOS 11
- Updated Sparkle Updater library.
- Added new translations:
  - Brazilian Portuguese
- Updated existing translations.

### Windows
- Please note, the Windows UI now requires  "Microsoft .NET 5 Desktop Runtime"
- Windows 10 is now the minimum version supported. The app will still run on Windows 7 and 8.1 however you will receive a message noting that it is not supported (twice) after which it will continue to run. Please note some functionality may not work correctly on these older operating systems and no support will be provided.
- Process Isolation
  - When enabled, any encodes that are started are run under a separate "handbrake.worker.exe" process. 
  - This protects the main UI from any crashes that could occur whilst processing a file and allows the queue to continue.
  - Multiple jobs can be run simultaneously to improve CPU utilisation on high core count systems.
- Audio Tab
  - Minor improvement to the usability of auto-passthru. To prevent confusion, the "auto-passhtru" audio "encoder" option is now only available on the defaults screen and not the main audio tab. 
- Queue Improvements
  - Restored lost "Stop" functionality in the new queue design that landed in 1.3. 
- Presets
  - The legacy preset side panel has been removed. The presets button on the toolbar will now offer a "preset manager" and list the available for selection.
  - The inline-preset view from 1.3 is now permanent. 
- Installer Improvements
  - Existing NSIS installer: Option to create a shortcut for "all users" as the last step. 
  - An MSI based installed (mostly for system-admins) is now available to allow for easier deployments. Feedback welcome.
- Preference Updates: 
  - New Auto Name option: Always use the default path for each new source / title selected
  - "Send File To" Arguments now supports "{source}" and "{destination}" replacement placeholders. 
  - Added a preference to configure the "Pause on Low Battery" feature. 
  - Check for Updates is now "opt-in" for new installs. 
- UI Performance: Optimisations to allow better performance when handling large sets of files (1000+)
- Added new translations (levels of completeness vary):
  - Brazilian Portuguese
  - Corsican
  - Italian
  - Japanese
  - Persian (Iran)
  - Thai
  - Ukrainian
- Updated existing translations (levels of completeness vary).
- Miscellaneous bug fixes and improvements


## HandBrake 1.3.3

### All platforms

#### General

- Fixed ISO 639-2/B language codes not set correctly in MKV (affects Hebrew, Indonesian, Javanese, and Yiddish) (#2903)
- Improved support for sources where pixel format cannot be quickly identified, e.g. due to delayed video track start (#2893)
- Added logging to identify where hardware support is disabled
- Miscellaneous bug fixes and improvements

#### Video

- Improved Intel QSV memory footprint by eliminating a buffer pool (#2675)
- Improved Intel QSV H.265 memory buffer size as required by newer Intel Media SDK (#2862)
- Fixed and improved Intel QSV in various situations, especially hardware decoding (#873, #2660, #2661, #2829)
- Fixed full range video being not being identified as limited range after conversion where filters are used (#2859)

#### Subtitles

- Fixed handling of overlapping SSA import subtitles (791adbac)
- Improved support for out-of-order SSA subtitles as allowed by specification (#2906)

#### Command line interface

- Fixed --preset failure unless full path is specified, e.g. --preset="Category Name/Preset Name" (#2838)

#### Build system

- Improved Flatpak to better conform to freedesktop metainfo standards
- Improved Intel QSV Flatpak plugin build efficiency using cmake-ninja
- Added a patch to fix cross compiling libdav1d using GCC 10.x (quality of life improvement)
  - Official HandBrake 1.3.3 Windows release is built using GCC 9.x and is not directly affected by this issue

#### Third-party libraries

- Updated libraries
  - FFmpeg 4.2.3 (decoding and filters)

### Mac

- Fixed preview layout not displaying properly on OS X 10.11 El Capitan
- Fixed incorrect copyright year on About dialog (#2830)

### Windows

- Fixed a crash related to the dark theme (#2816)
- Fixed a potential crash related to preview image memory allocation (#2871)
- Fixed a potential crash due to certain actions causing no preset being selected (#2875)
- Fixed missing E-AC-3 encoder option (#2855)
- Fixed hardware encoder support unavailable in portable build (#2832)
- Miscellaneous bug fixes and improvements


## HandBrake 1.3.2

### All platforms

#### General

- Fixed point to point end detection in certain scenarios (#2603)
- Improved support for H.265 video in AVI container produced by some security cameras (#2622)
- Added logging to identify problematic sources where container and video track pixel aspect ratios differ
- Added logging to help debug potential JSON API issues

#### Video

- Fixed color range conversion being applied twice when scaling video (#2561)
- Fixed incorrect identification of support for QSV HEVC encoder on older Intel hardware (#2558)
- Added logging to identify automatic picture rotation
- Miscellaneous bug fixes and improvements

#### Audio

- Fixed an upstream FFmpeg issue where passing through AAC ADTS audio could produce invalid MKV output in rare cases (error instead) (#2809)

#### Filters

- Fixed uninitialized memory in NLMeans prefilter leading to video corruption at bottom of picture (only affects custom settings) (#2576)
- Fixed a crash in the Detelecine filter with out of bounds parameters (only affects custom settings) (#2560, #2804)

#### Subtitles

- Fixed burned in subtitles position offset where cropscale filter is not used (#2449)

#### Command line interface

- Fixed subtitles not being selected when specifying --all-subtitles without also specifying a non-empty --subtitle-lang-list

#### Build system

- Fixed building the GTK graphical interface for use on Windows (link ole32)
- Updated Flatpak manifest creation script for compatibility with Python 3
- Updated Flatpak runtime and numactl library versions, QSV plugin
- Improved minimum version dependencies to facilitate building on systems with older automake and pkg-config
- Added a workaround for an upstream libdav1d issue affecting installation on FreeBSD (#2662)
- Miscellaneous bug fixes and improvements

### Linux

- Fixed point to point controls not accepting fractional seconds
- Fixed updating presets with identical names in different categories
- Improved parity with other platforms by allowing removal of preset categories (automatic after last preset in category is removed)
- Improved parity with other platforms by showing title and chapter range on the queue summary tab

### Mac

- Fixed selection behavior new track audio mixdown set to DPL2 instead of stereo (#2641)
- Fixed queued job failures related to removable drives by resolving security scoped resources as needed (#2566)

### Windows

- Fixed loading preset files with Unicode characters in path (#2427)
- Fixed clear queue options to prevent them clearing active jobs (#2587)
- Fixed main window status label not always reflecting the true count of queue jobs (#2538)
- Fixed failure loading default settings which could cause various issues in the graphical interface (#2549)
- Fixed preview images displaying incorrectly in some cases (anamorphic none, flip horizontal) (e9675bb, #2764)
- Fixed various issues related to Auto Passthru, including fallback settings (#2619, #2627, #2611)
- Fixed exported presets not importing correctly using the Mac graphical interface (#2531)
- Fixed pause and resume not working correctly in some cases (#2647)
- Fixed display of times greater than 24 hours (estimated time renaming, paused duration) (#2582, #2649)
- Fixed various cosmetic issues in the graphical interface (#2645, #2646)
- Improved display of long filenames in the queue (#2570)
- Improved some UX stress cases related to the queue (#2632)
- Improved error message when importing a preset specifying a nonexistent audio encoder (#2638)
- Improved audio and subtitle languages behavior to preserve selected languages order where "any" is also selected (#2611)
- Improved low disk space preferences and alerts (#2648)
- Added a workaround for an upstream .NET issue causing tooltips to not render correctly in some cases (#2630)
- Miscellaneous bug fixes and improvements


## HandBrake 1.3.1

### All platforms

#### General

- Fixed potential crash when opening a DVD source

#### Video

- Fixed rotation/flip not working properly in some cases
- Fixed an issue with QSV failures when using --start-at
- Updated presets using x265 to set aq-mode 1, the default prior to HandBrake 1.3.0
- Improved AMD VCE rate control by always explicitly setting the rate control method
- Added a workaround to fix x265 not parsing the H.265 Level setting where localized
- Added an upstream patch to fix x265 limit-tu bug in loading co-located CU's TU depth
- Added an upstream patch to fix x265 2-pass encoding failure
- Added an upstream patch to fix x265 VBV macroblocking at end of final GOP

#### Audio

- Fixed importing older presets where "und" was used to select any language track
  - Since HandBrake 1.3.0, "any" selects any language track and "und" selects undefined language tracks only
- Fixed secondary audio tracks on Blu-ray sources not being detected in some cases

#### Subtitles

- Fixed importing older presets where "und" was used to select any language track
  - Since HandBrake 1.3.0, "any" selects any language track and "und" selects undefined language tracks only

#### Build system

- Fixed cpp and lib flags causing build failures on some Linux systems

#### Third-party libraries

- Updated libraries
  - FFmpeg 4.2.2 (decoding and filters)

### Linux

- Fixed UI translations not working in some cases
- Fixed display of chapter start times
- Fixed small memory leak in audio list

### Mac

- Fixed importing external ASS/SSA subtitles
- Fixed statistics not updating after queue completion
- Updated documentation link to the most recent documentation version

### Windows

- Fixed missing UI translations for some languages
  - Español (Spanish)
  - Français (French)
  - 한국어 (Korean)
  - русский (Russian)
  - Türkçe (Turkish)
- Fixed official presets not updating after installing a new release
- Fixed preference for automatically naming file extension MP4/M4V not working
- Fixed an issue preventing the use of relative paths for automatic naming
- Fixed audio and subtitles selection behavior not saving when set via the Save New Preset dialog
- Fixed closed captions not being added automatically per selection behavior
- Fixed iPod 5G support option displaying when an incompatible encoder is selected
- Fixed queue updating slowly or not updating in some cases
- Fixed a few UI issues and a crash with the new dark theme.
- Fixed window not restoring properly after minimizing to system tray
- Fixed frame rate mode not updating on video codec change, preventing QSV zero-copy mode


## HandBrake 1.3.0

### All platforms

#### General

- HandBrake is now translated into many more languages
- Redesigned queue UI
- Removed Windows Mobile presets
  - See the [list of compatible replacements on GitHub](https://github.com/HandBrake/HandBrake-docs/blob/03682bdd741cea425c80b06818e4bdaec75bdc5e/source/docs/en/latest/technical/official-presets.markdown#windows-mobile-presets)
- Improved log output by silencing many lines at standard log level
- Improved quality of Gmail presets slightly
- Added Playstation 2160p60 4K Surround preset (supports PS4 Pro)
- Added Discord and Discord Nitro presets

#### Video

- Updated Intel Quick Sync Video to use Direct3D 11 API
- Updated minimum title scan duration to only apply to disc-based sources like Blu-ray and DVD
- Improved detection of MPEG-1 video in program streams
- Improved interface to FFmpeg avfilter and color handling
- Improved Nvidia NVENC constant quality encoding slightly by not setting qmin and qmax
- Replaced pp7 Deblock filter with FFmpeg Deblock filter
- Added support for reading Ultra HD Blu-ray discs (without copy protection)
- Added support for reading AV1 via libdav1d
- Added encoding to WebM container format
- Added Chroma Smooth filter (CLI only)
- Added zero-copy path for Intel QSV encoding removed in a previous release
- Added support for Intel QSV low power encoding (lowpower=1)
- Added support for AMD VCE encoding on Linux via Vulkan
- Added ability to select x265 encoder level and Fast Decode tune

#### Audio

- Updated audio resampling code to use FFmpeg swresample instead of libsamplerate
- Added source audio bit rate to tracks list
- Added ability to select unknown language tracks
- Added automatic track name pass through

#### Subtitles

- Added ability to import external SSA/ASS subtitles
- Added ability to select unknown language tracks
- Added automatic track name pass through

#### Command line interface

- Added additional unit aliases to --start-at and --stop-at, notably seconds and frames

#### Build system

- HandBrake now builds with libnuma on Linux
- Fixed Python bytestrings causing newlines to be ignored in build output
- Fixed Xcode ignoring make jobs parameter and utilizing all CPU cores (macOS only)
- Updated configure to enable most hardware encoders by default where appropriate by platform
- Updated all scripts for compatibility with Python 3
- Updated mac-toolchain-build script with newer tool versions many improvements
- Updated mingw-w64-build script with mingw-w64 6.0.0, GCC 9.2, and many improvements
- Improved compatibility with GCC 9 and recent Clang releases
- Improved compatibility with recent Xcode releases (macOS only)
- Improved host/build semantics
- Improved namespace isolation
- Improved handling of all options passed to configure
- Improved configure help output
- Improved detection of missing executable dependencies during configure
- Added support for building on NetBSD
- Added --harden configure parameter to enable buffer overflow protections
- Added --sandbox configure parameter to enable sandbox build target on macOS
- Added --enable-gtk4 configure parameter to build with GTK 4 instead of GTK 3
- Added summary of build options to configure output
- Miscellaneous bug fixes and improvements

#### Third-party libraries

- Removed libraries
  - libsamplerate (audio resampling)
- Updated libraries
  - AMF 1.4.9 (AMD VCE encoding)
  - bzip2 1.0.8 (general)
  - FDK AAC 2.0.1 (AAC audio encoding, must compile from source)
  - FFmpeg 4.2.1 (decoding and filters)
  - FreeType 2.10.1 (subtitles)
  - Fribidi 1.0.7 (subtitles)
  - HarfBuzz 2.6.4 (subtitles)
  - Jansson 2.12 (JSON architecture)
  - libbluray 1.1.2 (Blu-ray decoding)
  - libdav1d 0.5.1 (AV1 decoding)
  - libdvdnav 6.0.1 (DVD decoding)
  - libdvdread 6.0.2 (DVD decoding)
  - libiconv 1.16 (character encoding support)
  - libmfx (Intel QSV support)
  - libogg 1.3.4 (Xiph codecs support)
  - libopus 1.3.1 (Opus audio encoding)
  - libvorbis 1.3.6 (Vorbis audio encoding)
  - libvpx 1.8.1 (VP8/VP9 video encoding)
  - libxml2 2.9.9 (general)
  - nv-codec-headers 9.0.18.1 (Nvidia NVENC encoding)
  - x265 3.2.1 (H.265/HEVC video encoding)

### Linux

- Fixed slider control not showing complete values
- Updated translations (levels of completeness vary):
  - Czech
  - Chinese
  - French
  - German
  - Italian
  - Japanese
  - Korean
  - Norwegian
  - Russian
  - Spanish
  - Thai
- Added translations (levels of completeness vary):
  - Afrikaans
  - Basque
  - Croatian
  - Dutch
  - Polish
  - Portuguese
  - Romanian
  - Slovak
  - Swedish
  - Turkish
  - Ukrainian
- Added Intel QSV support to Flatpak (requires additional plugin installation)
- Added ability to double-click to edit audio track settings
- Added options to open encode log and log directory to actions menu on queue window
- Miscellaneous bug fixes and improvements

### Mac

- HandBrake now requires OS X 10.11 El Capitan or later
- HandBrake is now sandboxed and uses the macOS hardened runtime
- Updated priority for low-priority threads on macOS to avoid potential stalls in future macOS
- Updated translations:
  - German
- Added translations:
  - French
  - Italian
  - Russian
- Added preference to disable preview image on summary tab
- Miscellaneous bug fixes and improvements

### Windows

- Improved browse dialog recently used to fallback to parent directory
- Improved preferences layout
- Improved auto naming collision behavior and file overwriting
- Added preference to test selected notification sound
- Added preference to perform when done action immediately without countdown
- Added preference to disable preview image on summary tab
- Added hardware.enabled option to portable.ini
- Added dark theme for Windows 10
- Added queue import/export removed in a previous release
- Added new low battery level and disk space safety measures
  - Encoding jobs automatically pause when battery level is low, system sleep is allowed, and jobs resume when power is restored
  - Encoding jobs automatically pause when disk space drops to critical levels
- Added translations (levels of completeness vary):
  - French
  - German
  - Chinese
  - Korean
  - Russian
  - Spanish
  - Turkish
- HandBrake now requires .NET Framework 4.7.1 or later (installer offers download if missing)
- Miscellaneous bug fixes and improvements


## HandBrake 1.2.2

### Mac

- Fixed built-in application updater

### Windows

- Fixed crash on first launch for new installs, or older installs without settings.json
- Fixed an issue where the summary tab wasn't always up-to-date


## HandBrake 1.2.1

### All platforms

#### General

- Fixed potential crashes due to use of uninitialized variables
- Improved minimum duration to only apply to DVD and BD sources
- Miscellaneous bug fixes and improvements

#### Video

- Fixed incorrect video rotation where rotation is explicitly flagged as none
- Fixed yadif deinterlace filter not properly deinterlacing all frames
- Fixed missing frame at the end of encodes from m2ts sources
- Fixed detection of MPEG-1 video in program streams by improving probing of unknown streams
- Fixed decoding of MPEG-1 video in DVD sources
- Fixed Apple VideoToolbox encoding issues related to pyramidal B-frames
- Fixed lapsharp filter corrupting frame edges
- Improved NLMeans performance by reducing number of threads used with CPUs with high logical core counts
- Improved AMD VCE encoding to enable placing key frames at chapter markers
- Improved calculation of final frame duration
- Improved support for BT.2020 color space

#### Audio

- Fixed channel mapping for AAC 5.1 encoding (signal 5.1 Back instead of 5.1 Side which is less compatible)

#### Command line interface

- Fixed selection of encoders applied to tracks selected with `--all-audio`
- Fixed audio settings where more are specified than exist in the preset
- Fixed overriding audio bit rate set by the preset

#### Build system

- Fixed libvpx cross compilation with GCC 8 by disabling AVX-512
- Fixed x265 cross compilation with GCC 8
- Updated to mingw-w64-build 4.1.1 with improvements to error reporting and support for sha256sum on systems without shasum
- Miscellaneous bug fixes and improvements

#### Third-party libraries

- Updated libraries
  - libmfx (upstream API 1.27)

### Linux

- Fixed Flatpak icon validation by reducing resolution to pass new restrictions
- Fixed Flatpak accessing gvfs mounted filesystems by adding an additional access permission
- Fixed display of special characters in preset names
- Fixed exporting presets to sanitize system reserved characters 
- Miscellaneous bug fixes and improvements

### Mac

- Fixed incorrect file extension when selecting a preset
- Fixed potential user interface hang on macOS 10.12 Sierra
- Fixed potential issue caused by setting work state to done before all threads have closed
- Improved usability of preview controls overlay by increasing its size
- Miscellaneous bug fixes and improvements

### Windows

- Fixed application uninstaller sometimes appearing behind the installer
- Fixed potential crash when loading settings from older HandBrake versions
- Fixed official presets not updating when a newer HandBrake version is installed
- Fixed maximum resolution limit when selecting a preset (source or preset, whichever is smaller)
- Fixed crop and anamorphic settings not being restored when editing a queued job
- Fixed audio and subtitle selection behaviour where any language + first track are selected
- Restored options control on the queue window previously removed in HandBrake 1.2.0
- Improved user interface on displays close to the minimum recommended resolution
- Improved quality of text rendering on the installer for high density displays
- Improved ordering of presets and preset categories
- Improved QSV compatibility with newer Intel drivers by updating libmfx
- Improved removal of small temporary files that could be left behind in certain cases
- Miscellaneous bug fixes and improvements


## HandBrake 1.2.0

### All platforms

#### General

- Switched core decoding library from Libav to FFmpeg
  - Fixes numerous sources previously unreadable or otherwise broken
  - Facilitates a number of the improvements and features in this release and planned for the future
- Removed deprecated Legacy presets
  - See the [list of compatible replacements on GitHub](https://github.com/HandBrake/HandBrake-docs/blob/efb51cc2cd7d0c30fa5e9ee88366233ca34757a4/source/docs/en/latest/technical/official-presets.markdown#legacy-010x-presets)
- Updated official presets to use stereo instead of DPL2 mixdown
  - Avoids potential spatial positioning issues with the current DPL2 algorithm and wide pans in source material
  - Limited in impact since DPL2 decoding has not been in widespread use for years
- Updated official presets descriptions to revise compatibility and mention recently released devices
- Updated official presets to rename Fire TV to Amazon Fire
- Added Amazon Fire 720p30 and Chromecast 1080p60 presets
- Added `{creation-date}` and `{creation-time}` to automatic file naming

#### Video

- Fixed an issue decoding Blu-ray titles where the aspect ratio is unknown (assume 16:9)
- Fixed an issue encoding video with very short frame durations (less than 0.00285s or greater than 350 FPS)
- Improved extradata handling to accommodate all codecs
- Added support for decoding TIFF/LZMA video

#### Audio

- Fixed potential decoding issue for audio lacking an explicit channel layout (intelligently guess the layout)
- Fixed a potential crash during audio probe
- Improved resampling to allow dithering for all codecs (only where necessary)
- Improved quality of the default AAC encoder on non-Mac platforms (FFmpeg AAC), no longer experimental
- Improved bit rate constraints to allow Opus as low as 6 kbit/s per channel
- Added support for up to 7.1 channel AAC encoding (note that FDK AAC/HE-AAC do not support 6.1)
- Added support for E-AC3 audio in MP4 container
- Added Speex audio decoder

#### Subtitles

- Fixed a potential crash where an SRT file cannot be opened
- Added support for SRT files using periods instead of commas to delineate fractions

#### Command line interface

- Fixed inability to override preset subtitles burn setting (`native` and `none` are now valid values for `--subtitle-burned`)

#### Build system

- Fixed Linux packaging with an out-of-tree build directory
- Fixed Windows graphical interface build script signing tool location
- Removed `--enable-local-*` and associated contrib libraries; please see the [documentation](https://handbrake.fr/docs/) for dependencies help
- Updated to mingw-w64-build 4.1.0 with gcc 7.3.0, continuous output (keep alive), and miscellaneous improvements
- Improved support for building with Xcode 10
- Improved support for building on FreeBSD 11, 12, and 13
- Improved Flatpak packaging for Linux (numerous fixes and improvements, no longer experimental)
- Improved `configure.py` to always use the Python executable found by `configure`
- Added script for creating Flatpak manifests
- Added support for selecting a compiler via the CC environment variable
- Miscellaneous bug fixes and improvements

#### Third-party libraries

- Removed libraries
  - Libav 12.3 (decoding and filters)
  - yasm 1.3.0
- Updated libraries
  - libopus 1.3 (Opus audio encoding)
  - x264 157 r2935 (H.264/AVC video encoding)
  - x265 2.9 (H.265/HEVC video encoding)
- New libraries
  - FFmpeg 4.1 (decoding and filters)
  - liblzma (xz) 5.2.4 (LZMA video decoding, e.g. TIFF)
  - libspeex 1.2.0 (Speex audio decoding)

### Linux

- Fixed Blu-ray title name being set to device name (e.g. sr0) when scanning raw devices
- Fixed Blu-ray default destination file names to no longer include MPLS number
- Fixed an issue with queue state not being updated properly on reload
- Fixed various issues importing presets
- Updated most translations
- Added initial support for GTK 4
- Added ability to customize activity window font size and increased default from 7 to 8
- Added destination overwrite protection (append number to file name on conflict)
- Added `{source-path}` to automatic path setting
- Miscellaneous bug fixes and improvements

### Mac

- HandBrake now requires OS X 10.10 Yosemite or later
- Removed preferences option to show the advanced tab (deprecated and slated for removal)
- Fixed minor display issues on macOS 10.14 Mojave
- Fixed queue toolbar icon not updating
- Removed Growl in favor of native system notifications
- Updated Sparkle automatic update library
- Improved handling of invalid presets
- Improved layout to better support localization
- Improved preview border alignment on high density displays
- Improved automatic naming to avoid updating when unrelated settings change
- Improved criteria for showing destination overwrite warnings
- Improved stop encoding confirmation dialog
- Improved notifications to no longer play sounds when alerts are disabled
- Added initial localization support and German translation
- Added initial support for VideoToolbox hardware-accelerated encoding
- Added support for Dark Mode on macOS 10.14 Mojave (new Dark Mode toolbar icons require building with Xcode 10 on Mojave)
- Added Touch Bar support to various windows and dialogs
- Added Finder progress bar to files currently encoding
- Added Show Source in Finder to queue contextual menu (renamed Show in Finder to Show Destination in Finder)
- Added validation for custom filter parameters
- Miscellaneous bug fixes and improvements

### Windows

- HandBrake now requires .NET Framework 4.7.1 or later (installer offers download if missing)
- Fixed mixdown selection where the specified mixdown in the selected preset is not available
- Fixed audio sample rate of 48 kHz being selected where Auto was the specified behavior
- Removed preferences option to show the advanced tab (deprecated and slated for removal)
- Removed options menu from queue in favor of contextual menu
- Improved layout to better support localization
- Improved error handling when adding items to the queue
- Improved multi-instance queue recovery to avoid loading queue files from the wrong instance
- Improved filters layout and controls
- Improved video quality slider by allowing it to grow with window size and setting x264/x265 granularity to 0.5
- Improved SRT import default browse location (open source video location)
- Improved keyboard shortcuts by making Ctrl-S open the destination save dialog (start encode is now Ctrl-E)
- Added additional keyboard shortcuts for various actions
- Added initial localization support and German translation
- Added support for AMD VCE and Nvidia NVENC hardware-accelerated encoders
- Added automatic queue archiving and option to select an archived queue for recovery (archives are removed after 7 days)
- Added support for recovering specific queue files on start using `--recover-queue-ids=<id1,id2,...>`
- Added additional controls to passthru audio tracks to configure audio fallback parameters
- Added ability to drag and drop SRT subtitles files onto the main window
- Added option to disable preview image on summary tab
- Added option to disable checking for update on start in portable mode
- Added stop encoding confirmation dialog
- Miscellaneous bug fixes and improvements


## HandBrake 1.1.2

### All platforms

#### General

- Fixed Blu-ray title name where reading from raw device
- Improved handling of broken units in Blu-ray source (skip and continue reading)

#### Video

- Fixed rare crash related to preview images, anamorphic, and default preset values
- Fixed jitter produced by 59.94 fps to 29.97 fps frame rate conversion

#### Audio

- Fixed duration of silence buffers (may improve audio/video sync for some sources)

#### Build system

- Fixed linker warning by setting libvpx target to minimum supported macOS version
- Fixed build failure with old GTK versions
- Miscellaneous Flatpak-related fixes and improvements

#### Third-party libraries

- Updated libraries (necessary to pull in needed bug fixes)
  - libdvdread 6.0.0 (DVD decoding)
  - libdvdnav 6.0.0 (DVD decoding)

### Linux

- Fixed potential crash where $HOME directory is missing
- Fixed legacy preferences causing preset window being shown at every launch
- Fixed preset name and menu display on Ubuntu 14.04 Trusty Tahr
- Fixed default destination file name for Blu-ray source to not include MPLS number
- Fixed queue reloading in Flatpak sandbox
- Fixed toolbar icons alignment and use correct art for lower pixel density
- Updated most translations

### Mac

- Fixed menu item and tooltip strings related to selection behavior
- Fixed crash where presets file contains invalid UTF-8

### Windows

- Fixed incorrect track selection behaviours on audio and subtitle tabs
- Fixed creating an unneeded HandBrake Team directory in the user's Roaming directory
- Improved path handling when opening a source directory from the queue
- Miscellaneous bug fixes and improvements


## HandBrake 1.1.1

### All platforms

#### General

- Fixed a potential crash in the comb detection filter
- Fixed a potential crash in the padding filter

#### Video

- Fixed decoding certain very high bit rate ultra-high definition sources with extra large packets
- Fixed last frame in source video missing in output

#### Audio

- Fixed crash when decoding an empty audio track

#### Build system

- Updated mac-toolchain-build script Nasm url and improved curl parameters for robustness

#### Third-party libraries

- Updated libraries (necessary to pull in needed bug fixes)
  - libvpx 1.7.0 (VP8/VP9 video encoding)
  - x264 155 r2901 (H.264/AVC video encoding)

### Linux

- Fixed Ubuntu PPA build date
- Fixed a display issue with the quality slider control
- Fixed issues occurring when running multiple HandBrake instances simultaneously
- Updated Russian translation
- Updated support for creating Flatpak bundles (experimental)
- Miscellaneous bug fixes and improvements

### Mac

- Fixed corrupted output with VP8 and VP9 encoders
- Fixed building x264 using Clang and -march=native/-mavx (thanks H. Gramner for the upstream patch)

### Windows

- Fixed issues with queue recovery when running multiple HandBrake instances simultaneously
- Fixed an issue with a When Done action potentially causing the last queue item to be marked as a warning instead of success
- Fixed burn-in behavior with MP4 files; after the first burn-in track, no additional tracks that require burn-in will be added
- Improved window resize behavior for Add Selection to Queue dialog and allowed space bar to toggle checkbox for selected row
- Improved tabbing behavior in various circumstances
- Improved adding a new preset with the same name as an existing user preset; the existing preset will now be updated properly
- Improved privacy by adding an option to disable the Summary tab preview image
- Updated installer to block installation on 32-bit systems to avoid confusion (unsupported since 1.1.0)
- Miscellaneous bug fixes and improvements


## HandBrake 1.1.0

### All platforms

#### General

- Improved user interface
  - Redesigned main window for consistency and usability; overall flow is top to bottom, left to right
  - Added new preset controls directly in main workflow
  - Added new summary tab for overview of settings at a glance
  - Updated interface icons to support high resolution displays
- New and improved official presets
   - Added new presets for Vimeo and YouTube
   - Added new 2160p/4K device presets for Apple TV, Chromecast, Fire TV, and Roku
   - Added new Production presets for post-production video editing workflows
   - Added additional 2160p/4K Matroska presets
- Improved AppleTV 4K support
- Improved Intel QuickSync Video support
  - Added Linux support (experimental)
  - Added 10-bit H.265/HEVC encoding support for Kaby Lake and newer CPUs
  - Added support for multiple GPUs via D3D11 encode path
- Many bug fixes and improvements

#### Video

- Added new Unsharp and kernel-based Laplacian LapSharp sharpening filters
- Added CSM prefilter option to NLMeans filter
- Added support for mobile device orientation via auto-rotation container flag
- x265 10- and 12-bit encoders are now included by default; additional dll files are no longer required

#### Command line interface

- Added support for adaptive streaming (SPS and PPS before IDR frames) via the --inline-parameter-sets parameter
- Added --json parameter to output scan/status information in JSON format, useful for scripting
- Audio sample rate parameter --arate now accepts "auto" as a valid value

#### Build system

- Added support for FreeBSD 11.1 (must compile from source)
- Added support for compiling on Windows under MSYS (experimental, slow)
- Updated to mingw-w64-build 3.1.1 with support for mingw-w64 5.0.3 and gcc 7.2.0
- Update mac-toolchain-build to add NASM 2.13.2 (now required for x264)
- Miscellaneous bug fixes and improvements

#### Third-party libraries

- Updated libraries
  - FDK AAC 0.1.5 (AAC audio encoding, must compile from source)
  - FreeType 2.8.1 (subtitles)
  - HarfBuzz 1.7.2 (subtitles)
  - Jansson 2.10 (JSON architecture)
  - LAME 3.100 (MP3 audio encoding)
  - Libav 12.3 (decoding and filters)
  - libass 0.14.0 (subtitles)
  - libbluray 1.0.2 (Blu-ray decoding)
  - libdvdnav 5.0.3 (DVD decoding)
  - libdvdread 5.0.3 (DVD decoding)
  - libiconv 1.15 (character encoding support)
  - libmfx (upstream API 1.23)
  - libogg 1.3.2 (Xiph codecs support)
  - libopus 1.2.1 (Opus audio encoding)
  - libsamplerate 0.1.9-35-g02ebb9f (audio resampling)
  - libtheora 1.1.1 (Theora video encoding)
  - libvorbis 1.3.5 (Vorbis audio encoding)
  - libvpx 1.6.1 (VP8/VP9 video encoding)
  - libxml2 2.9.4 (general)
  - x264 155 r2893 (H.264/AVC video encoding)
  - x265 2.6 (H.265/HEVC video encoding)
  - zlib 1.2.11 (general)

### Linux

- Added option to configure low disk space warning level
- Added Intel QuickSync Video encoder (experimental, requires specific Intel driver)
- Added support for Ubuntu 18.04 and 17.10; Ubuntu 15.04 support is removed
- Many other bug fixes and improvements

### Mac

- Added option to configure low disk space warning level
- Improved support for VoiceOver navigation
- Many other bug fixes and improvements

### Windows

- Added option to configure low disk space warning level
- Added option to play a sound (MP3/WAV) when encode or queue is finished
- Added option to show progress, pass, passcount in the application title or task tray icon tooltip
- Added more granular progress reporting during search for start of file and muxing
- Added support for per-display resolution awareness
- Added support for running HandBrake in a portable mode (see included portable.ini.template)
- Added custom anamorphic to dimensions tab, it's back!
- Improved static preview window still previews rendering
- Improved audio selection behavior controls; dropdowns are now context aware and range limited with better defaults
- Improved UI consistency throughout
- Many other bug fixes and improvements


## HandBrake 1.0.7

### All platforms

#### Video

- Fixed decoding of raw video in Matroska/MKV
- Fixed time stamp handling for containerless raw video
- Fixed memory leaks in OpenCL
- Fixed x265 2-pass encoding where video frame rate is altered by filters
- Improved handling of very small amounts of sync jitter (~1 tick)
- Improved handling of AVI files with broken indices

#### Audio

- Further refined fix for Libav crash encoding AAC at very high bitrates

#### Subtitles

- Removed fontconfig dependency on Mac and Windows; libass now uses CoreText/DirectWrite
- Improved subtitle font selection when burning in SRT/SSA subtitles

#### Command line interface

- Fixed preset subtitle burn defaults override

#### Build system

- Updated MinGW-w64 build script for improved compatibility with hardened GCC

### Linux

- Fixed video preset control not updating
- Fixed audio passthru masks not updating until preset save
- Fixed application of SRT subtitles offset
- Updated translations: af, cs, ja_JP, ru, sk, sv

### Windows

- Fixed When Done option not updating/resetting correctly


## HandBrake 1.0.6

Superseded by HandBrake 1.0.7.


## HandBrake 1.0.5

Superseded by HandBrake 1.0.6.


## HandBrake 1.0.4

Superseded by HandBrake 1.0.5.


## HandBrake 1.0.3

### All platforms

#### Video

- Fixed H.264 decoding using Libav where the initial GOP was dropped
- Fixed 2-pass x265 encoding where the source header incorrectly specifies frame rate
- Fixed 2-pass encoding with bob deinterlace and constant frame rate
- Fixed a seek issue in Libav while reading MKV sources with embedded subtitles
- Fixed multiple issues preventing Libav from opening WMV sources properly
- Fixed miscellaneous issues in Libav
- Fixed memory leaks in OpenCL
- Improved sync for streams delayed by a large amount

#### Audio

- Fixed a Libav crash encoding AAC at very high bitrates
- Fixed a potential hang in Libav while decoding AAC
- Improved Libav audio sync with MP4 sources containing edit lists
- Improved mapping of single channel layouts to single channel layouts

### Linux

- Fixed a potential crash when selecting video encoders
- Fixed various controls not applying values properly

### Mac

- Fixed a crash when attempting to edit a queued job
- Improved audio start point to use edit lists when encoding using CoreAudio AAC

### Windows

- Fixed Title Specific Scan for DVD/Blu-ray
- Fixed broken/missing NLMeans denoise tunes
- Fixed an issue that could cause error -17 when encoding using Intel QuickSync Video
- Fixed an issue where the SRT language code was not being set correctly


## HandBrake 1.0.2

### All platforms

#### Video

- Fixed point to point encoding end point when using frames as the unit
- Improve error handling for libdvdread and libavcodec decoders

#### Audio

- Fixed an issue where fallback encoder bitrate was not set properly

#### Subtitles

- Fixed incorrect duration for UTF-8 subtitles
- Fixed an issue causing extra blank lines for UTF-8 subtitles in MKV

### Linux

- Fixed an issue sometimes preventing dragging and dropping of presets between folders
- Miscellaneous bug fixes

### Mac

- Fixed video encoder options not reset when changing encoders
- Fixed incomplete encodes where the chapter count differs on queued items
- Fixed sleep prevention not working in certain scenarios
- Fixed automatic naming for EyeTV bundles
- Added additional checks to prevent source file overwriting
- Miscellaneous bug fixes

### Windows

- Fixed video encoder options not reset when changing encoders
- Fixed subtitles defaults behaviors to improve how tracks are added
- Fixed issues related to source maximum and custom picture setting modes
- Fixed point to point settings when editing a queued job
- Fixed When Done controls not updating correctly on various screens
- Fixed split buttons not working correctly on audio and subtitles tabs
- Fixed destination path and file name error checking
- Fixed potential crash by disabling OpenCL detection when the scaler is not set to OpenCL Bicubic (workaround for broken system drivers)
- Fixed a potential crash when QSV is supported but disabled at the BIOS level
- Fixed memory leaks in the QSV encoder
- Added QSV detection for Intel Kaby Lake CPUs
- Miscellaneous bug fixes


## HandBrake 1.0.1

### All platforms

#### Video

- Fixed a potential crash when using the VP8 or VP9 video encoders
- Fixed a potential crash when using 2-pass ABR

#### Command line interface

- Fixed importing chapter names from a file

### Linux

- Fixed Keep Aspect Ratio control (disabled) when anamorphic mode is Auto
- Widened presets list to accommodate longer preset names
- Miscellaneous bug fixes

### Mac

- Fixed a potential crash when using the subtitles burn feature on macOS versions earlier than 10.12 Sierra
- Fixed certain controls not updating when saving or loading a preset
- Miscellaneous bug fixes

### Windows

- Fixed certain controls not updating when saving or loading a preset
- Disabled QSV decoding by default when non-QSV encoder selected (configurable)
- Miscellaneous bug fixes


## HandBrake 1.0.0

### All platforms

#### General

- New online documentation at https://handbrake.fr/docs
- Completely overhauled the official presets
  - New general use presets for broad compatibility
  - New device presets, now more up-to-date for common devices
  - New web presets
  - New Matroska (MKV) presets, including VP9 video with Opus audio
  - Official presets from HandBrake 0.10.x are still available under `Legacy`
- New JSON-based preset system including command line support
- New JSON-based API for interacting with libhb
- Improvements to audio/video sync engine to better handle difficult sources
- Many miscellaneous bug fixes and improvements (over 1700 code commits!)

#### Video

- VP9 video encoding via libvpx
- Intel QuickSync Video H.265/HEVC encoder
  - Requires Intel Skylake or newer CPU
- Ultra HD / 4K color pass through (support for BT.2020)
- Additional standard frame rate selections in the graphical interfaces
- New Auto anamorphic mode maximizes storage resolution, replaces Strict anamorphic mode
- New Pad filter (command line only for now)
- New Decomb/Deinterlace filter settings and improved defaults
- Rotate filter now available in all graphical interfaces
- New NLMeans filter tunes Tape and Sprite for analog tape recordings and vintage video games, respectively
- Assembly optimizations NLMeans filter improve performance up to 10%
- Assembly optimizations in x264 encoder improve performance for faster presets by 5-10%
- x265 encoder quality improvements, especially when using tune grain
- High bit depth encoding support via external shared libraries (video pipeline is still 8-bit 4:2:0)
  - x264 10-bit
  - x265 10-bit and 12-bit

#### Audio

- Opus audio encoding/decoding via libopus
- Passthru now supports E-AC-3, FLAC, and TrueHD audio formats

#### Subtitles

- Improved subtitles rendering for some languages via HarfBuzz
- Miscellaneous subtitles improvements

#### Command line interface

- Presets can now be imported and exported from the command line and are compatible with the graphical interfaces
- Queue exported from the graphical interfaces can now be imported by the command line interface (Linux and Windows only for now)

#### Build system

- Add scripts to manually build and install Mac and MinGW-w64 (compile for Windows on Linux) toolchains
- Add support for multiple source URLs for third-party downloads
- Add SHA256 hash verification for third-party downloads
- Add configure parameter to disable or filter allowed third-party downloads (see configure --help)
- Use HTTPS everywhere; the few cases where a third-party does not provide packages over https, handbrake.fr does
- New targets on Mac to install and uninstall after building
- Add flatpak packaging support (experimental)

#### Third-party libraries

- Updated libraries
  - FreeType 2.6.5 (subtitles)
  - Fontconfig 2.12.1 (subtitles)
  - FriBidi 0.19.7 (subtitles)
  - Libav 12 (encoding/decoding/muxing)
  - libass 0.13.2 (subtitles)
  - libbluray 0.9.3 (Blu-ray decoding)
  - libmfx v6.0.0 (Intel QuickSync Video encoding/decoding)
  - libvpx 1.5.0 (VP8/VP9 video encoding)
  - x264 148 r2708 (H.264/AVC video encoding)
  - x265 2.1 (H.265/HEVC video encoding)
- New libraries
  - HarfBuzz 1.3.0 (subtitles)
  - libopus 1.1.3 (Opus audio encoding)

### Linux

- Add options for saving and loading queue files
- Removed system tray icon due to performance issues on Ubuntu
- Usability improvements
- Miscellaneous bug fixes

### Mac

- Updated all tooltips
- Added undo/redo support to the graphical interface
- Improved drag and drop support
- Added Open Recent to the File menu
- Added Add Titles to Queue… to the File menu (batch queueing)
- Preview prompts to open in an external application when the internal player does not support the format
- Preview now displays volume and audio/subtitles language selection during playback
- Picture and Filters settings are now part of the main window
- Settings are preserved when selecting a new title (instead of reloading the last selected preset)
- Improved support for importing/exporting comma-separated (.csv) chapter markers
- Queue is now automatically paused when available disk space is low
- When Done action can now be changed directly from the Queue window
- When Done notification is now interactive (reveals the encoded file in Finder)
- Activity Log window is now searchable (press `⌘`+`f` to activate)
- XQuartz is no longer required for subtitle burn-in
- Updated Sparkle software update library
  - Enabled DSA signature checking for improved security
- Usability improvements
- Miscellaneous bug fixes

### Windows

- Graphical interface now uses libhb directly, instead of sending commands to the command line interface
  - Encoding can now be paused and resumed
  - Stopping encoding will finalize the partial file to be playable
- Reduced installer and install size
  - The command line interface is no longer included in the graphical interface installer
- Update checker now verifies the signature of the update file for improved security
- Added support for quality-based audio encoding
- Added ability to import tab-separated (.tsv), XML (.xml), and plain text (.txt) chapter markers
- Improved the "configure default" options for audio and subtitles
  - Default audio track behavior is now configurable
  - Subtitles burn-in behavior is now configurable
- Removed DirectX Video Acceleration (DXVA) hardware-accelerated video decoding
  - Was causing many issues without providing sufficient improvement to decoding efficiency
  - May be added again at a later date if performance and stability improves
- Usability improvements
- Miscellaneous bug fixes


## HandBrake 0.10.5

- Removed FDK AAC from binary releases
  - Configure with `--enable-fdk` for non-free and non-redistributable build when compiling from source
- Updated x265 to 1.9 which brings bug fixes and performance improvements
- Improvements in large AVI file handling
- Assorted bug fixes and performance improvements

## HandBrake 0.10.3

- Updated x265 to 1.8 which brings numerous bug fixes and some performance improvements
- Assorted bug fixes and performance improvements

## HandBrake 0.10.2

- Assorted bug fixes and performance improvements

## HandBrake 0.10.1

- Assorted bug fixes for all three GUIs and the core library

## HandBrake 0.10.0

### Core

- Intel QuickSync Video encode and decode support (beta)
  - Windows only currently; we hope to bring this to Linux users in the future
- DXVA hardware decode support (experimental)
  - Windows only
  - Suitable only for slower machines so disabled in preferences by default
- Choice of scalers
  - Lanczos
    - HandBrake's default scaler
  - Bicubic (OpenCL) (experimental)
    - Available on the Command Line Interface (all platforms) and Windows GUI (Mac / Linux GUIs will come in a later release)
    - Currently only available in OpenCL form so requires an AMD or Intel GPU supporting OpenCL 1.1 or later; Nvidia GPUs are not currently supported
    - When downscaling, up to 5% performance improvement can be achieved; no benefit when not downscaling
    - Small loss in quality over the Lanczos scaler
- Denoise
  - hqdn3d filter now accepts individual settings for each chroma channel (Cb, Cr)
  - New NlMeans denoiser, very slow, but results are significantly better than hqdn3d
- Presets
  - Added Windows Phone 8 Preset
- Updated Libraries
  - x264 r2479-dd79a61
  - Libav v10.1
  - libbluray 0.5.0
- libavformat is now used for muxing instead of mp4v2 and libmkv
  - "Large File Size" checkbox has now been removed for MP4, as the new muxer will transition to 64bit files automatically
  - mpeg2dec has also been replaced in favour of using libav
- The LibAV AAC encoder is now the default as FAAC has been remove
  - This encoder is adequate for most, but until it improves a bit further, we have enabled support for the FDK-AAC encoder also
    - This FDK option is a temporary measure until the LibAV encoder improves
    - Note that FDK-AAC is much slower and will likely bottleneck the encode process, but will produce better quality audio
- H.265 encoder
  - Now available through x265 1.4
  - While this encoder is still fairly new, we have seen some promising results come out of it; it's still under heavy active development and is only going to improve over time
- Added VP8 Encoder (using libvpx)
  - Available in MKV files only
- Removed mcdeint deinterlace and decomb modes (this relied on the snow encoder in libav which has been removed by upstream)
- Bug fixes and misc improvements

### Windows

- Accessibility and usability improvements
  - Added option to 'Use System Colors'; the app should now be usable in a high contrast mode.
  - Fixed tab ordering to make the app more keyboard friendly
- LibHB is now used for scanning instead of the CLI
  - Experimental preview window is now available as a result (can be enabled via preferences)
- Improved the layout and design of the audio and subtitle tabs
  - Split buttons similar to the old 0.9.8 WinForms GUI
  - Improved layout on the track listbox to make better use of the space
- Improvements to auto-naming feature
- When Done
  - Added an option that will reset this to 'Do nothing' when the app is closed and restarted
- Presets
  - New presets menu
  - Presets bar can now be hidden if it's not wanted
- Numerous bug fixes
  - Fixed the issue in the source dropdown where the drive menu items did not work when clicked
  - Numerous fixes in the picture settings panel around resolution calculation
  - Numerous fixes around the way presets are loaded and saved, particularly around picture settings
  - Removed Growl for Windows support due to bugs and issues with this library that remain unfixed; project appears abandoned
  - Many misc other fixes
- Windows XP is no longer supported; Windows Vista with Service Pack 2 or later is now a requirement
- The 32-bit build of the application is now considered deprecated; this is due to 32-bit process memory limitations

### Mac

- Build system updates to compile under OS X 10.9 and 10.10
- Automatic audio and subtitle track selection behaviors which can be stored per preset
- Improvements to auto-naming feature
- Misc UI enhancements
- Bug fixes and misc improvements

### Linux

- Automatic audio and subtitle track selection behaviors which can be stored per preset
- Improvements to auto-naming feature
- Batch add-to-queue by list selection
- Russian and Czech translations
- Bug fixes and misc improvements
- Requires GTK3

### Command Line Interface

- Basic support for return codes from the CLI (0 = No error, 1 = Canceled, 2 = Invalid input, 3 = Initialization error, 4 = Unknown Error)
- Bug fixes and misc improvements

## HandBrake 0.9.9

### General

- Improved HandBrake pineapple icon by Matt Johnson
- Improved Retina-resolution icons within the application, by [Nik Pawlak](http://nikpawlak.com)

### Core

- Blu-ray (PGS) subtitle support
  - works with Foreign Audio Search
  - can be Burned-In
  - can be passed through to MKV (but not MP4)
- Additional video framerates
  - 30, 50, 59.94, 60 fps
- Double framerate ("bob") mode for the deinterlace and decomb filters
- Better audio remix support
  - additional mixdowns: 6.1, 7.1, 7.1 (5F/2R/LFE)
    - CLI users should note 6ch becomes 5point1
  - better-than-Stereo sources can be upmixed to 5.1
  - discard one channel from Stereo sources
    - Mono (Left Only), Mono (Right Only)
- Allow the selection of higher audio bitrates where appropriate
- Allow the selection of lower audio samplerates where appropriate
  - 8, 11.025, 12, 16 kHz
- Audio dithering (TPDF) when converting to 16-bit FLAC
- Use libavcodec for DTS audio decoding (instead of libdca)
  - DTS-ES 6.1 Discrete support
- All graphical interfaces: support for x264's preset, tune and profile options
  - alternative to the Advanced panel (which is still available)
  - HandBrake-specific option to ensure compliance with a specific H.264 level
- Updated built-in presets
  - take advantage of x264 preset/tune/profile support
  - removed increasingly suboptimal and irrelevant Legacy presets
- Assorted bug fixes and improvements to the core library (libhb)
- Updated libraries
  - x264 r2273-b3065e6
  - Libav v9.6
  - libbluray 0.2.3

### Windows

- User Interface has been re-written in WPF
  - Includes many small UI enhancements
- Switched to .NET 4 Client Profile
  - smaller download for those who don't have .NET 4 Full installed

### Mac

- Assorted bug fixes
  - including better support for Retina displays
- Prevent sleep during encoding and scanning for Mountain Lion
- Drag & Drop files onto the Main window or application icon to scan
- Nicer progress indication on the dock icon
- Preview window improvements and bugfixes
- Updated Growl to 2.0.1
  - Notification Center support (when Growl is not installed)

### Linux

- Assorted bug fixes and improvements
- Use some system libraries rather than bundling
  - fontconfig, freetype, libxml2, libass, libogg, libvorbis, libtheora and libsamplerate

### Command Line Interface

- Audio option (-a) will ignore invalid input tracks and encode with only the valid ones
- Allow use of hh:mm:ss format when specifying p-to-p start/stop time
- Advanced audio options
  - enable level normalization when downmixing (disabled by default)
  - disable audio dithering or select a specific algorithm

## HandBrake 0.9.8

- Corrects a few crash bugs that showed up in 0.9.7

## HandBrake 0.9.7

- This is a bug fix release for 0.9.6.
- Includes an AppleTV3 Preset and updated iPad Preset

## HandBrake 0.9.6

### Encoders

#### Video

- updated libx264 (revision 2146)
- MPEG-2 encoder (from libavcodec)
- advanced options support for libavcodec encoders
  - format: option1=value1:option2=value2
    - -bf 2 -trellis 2 becomes bf=2:trellis=2

#### Audio

- audio gain control (increase/decrease audio volume)
- updated libogg (1.3.0) and libvorbis (aoTuV b6.03)
- new AAC encoder (from libavcodec) (considered experimental)
  - (supported mixdowns: Mono/Stereo/Dolby, 5.1 will come later)
  - (should be on par with faac in terms of quality, sometimes better)
- FLAC encoder (16-bit, MKV container only)
- Mac OS X: HE-AAC encoding support, via Core Audio
- quality-based variable bitrate encoding support
  - works with: Lame MP3, Vorbis, Core Audio AAC
  - only implemented in CLI and Linux GUI
- AC3 encoder: set Dolby Surround flag in stream parameters when mixdown is Dolby Surround or Pro Logic II

#### Audio Passthru

- DTS Passthru to MP4 container (in addition to MKV) (supported by e.g. VLC, MPlayer)
- DTS-HD Passthru (MP4, MKV containers)
- MP3 Passthru (MP4, MKV containers)
- AAC Passthru (MP4, MKV containers)
  - (known issue: Magic Cookie not passed through from MPEG Program/Transport streams, which will break playback in e.g. QuickTime Player)
- Auto Passthru: one encoder, multiple codecs
  - lets you define allowed codecs (from supported passthru codecs)
  - lets you pick a fallback encoder if passthru is not possible

#### Muxers

- start MKV clusters with a video keyframe whenever possible
  - should improve seeking and DLNA streaming
- bug fix: use ISO 639-2 bibliographic form for MKV language codes
- bug fix: fix crash due to division by zero in MP4 muxer
- bug fix: fix muxing of Closed Captions. Improper interleaving broke playback on some players

### Decoders

#### Video

- updated libav* libraries from Libav (http://libav.org/) (v0.7-1696-gcae4f4b, late October 2011)
  - frame-based multithreading for H.264, VP8
  - 10-bit decoding support for H.264, DNxHD
  - Apple ProRes decoding support
- improved average framerate detection
- duplicate frame detection for improved frame drop decision (CFR/PFR)
- new Same as source, Constant Framerate option
  - for devices that don't support variable framerate
  - automatically picks a constant framerate closest to the detected average framerate
- bug fix: fix problem when resolution changes in the middle of a video stream

#### Audio

- Blu-ray: make TrueHD, DTS-HD and E-AC3 Secondary Audio streams available for decoding and/or passthrough
- bug fix: libavcodec-decoded streams can now be decoded multiple times
  - previously, multiple decodes of the same source tracks weren't possible and audio output tracks had to be dropped
- bug fix: fix audio screech at beginning of some audio tracks when decoding AC3
- bug fix: fix DTS decoder audio volume (was too low)
- bug fix: garbled audio when decoding DTS-HD audio from MKV containers
- bug fix: fix support for DTS-HD High Resolution Audio in MPEG Transport streams

### Subtitles

- updated libass (0.10.0)
- improved handling of subtitles with overlapping timestamps
- improved handling of DVD subtitles without Stop Display commands
- SSA subtitles are now passed through to MKV without conversion to SRT/UTF-8
- bug fix: fix rendering problem with SSA subtitles when no font is embedded in the source video

### Demuxers

- improved MPEG Program/Transport stream support
  - support for MPEG-1 Program streams
  - support for HD-DVD EVOB streams
  - improved handling of Transport streams that have no PCR
- WTV container support (via libavformat)
- bug fix: files with more than 20 tracks (video, audio, subtitles etc.) are now supported
- bug fix: some QuickTime MOV files were misdirected as MPEG Transport streams
- bug fix: fix detection of TrueType font attachments that don't have the correct MIME type

### Filters

- new, much improved decomb filter (but slower)
  - new settings are default
  - old settings become decomb "Fast"

### Presets

- improved "Normal" preset (much faster, similar file size and quality)
- removed obsolete legacy presets
- added new device presets for Android phones/tablets
  - recent and/or powerful devices recommended

### Mac OS X

#### DVD decryption:

- VLC 2.x or later will not work for DVD decryption and is therefore unsupported as of HandBrake 0.9.6
- libdvdcss is now the preferred method for DVD decryption
  - already works with HandBrake 0.9.5
  - a .pkg installer is available from Videolan: <http://download.videolan.org/libdvdcss/last/macosx/>

#### Build system:

- support for Xcode 4 and Mac OS X 10.7 "Lion"
- Mac OS X 10.5 "Leopard" no longer supported

### Mac OS X GUI

#### OS X 10.7 Lion support

- bug fix: Live Preview window widgets updated to work under Lion
- bug fix: fixed positioning of widgets in the Audio panel under Lion

#### Other:

- wider main window providing more room for various widgets

### Windows GUI

#### Preview window

- complete redesign
- support for VLC or the system default video player
- dropped built-in QuickTime playback support

#### Other

- much improved control over the default audio and subtitle tracks selected (see Options)
- ability to set the minimal title length that will show up during a scan (see Options)
- several other usability improvements
- installer now has a silent option for easier network installs (launch the installer with /S)

### Linux GUI

#### Audio panel

- new advanced audio options section for gain and audio track names
- dynamic range compression and samplerate moved to advanced audio options

#### Other

- minor UI tweaks and usability enhancements

### Miscellaneous

- Target Size is gone, and isn't coming back
  - Don't bother complaining on the forums
- CLI: support for x264 presets, tunes and profiles
  - new --x264-preset, --x264-tune and --x264-profile options
- DVD: fix issues with some discs (e.g. True Grit, Thor, Transformers 3)
- DVD: improved main feature detection
- updated libbluray (0.0.1-pre-213-ga869da8, late May 2011)

## HandBrake 0.9.5

### Core Library

- BluRay disc structure support. (No decryption support)
- Updated Libraries (x264, ffmpeg)
- SSA Subtitle support. (Including burn-in)
- MP3 audio now supported in MP4 files (Note: Limited Player compatibility)
- VOBSUB subtitle now supported in MP4 files (Note: Limited Player compatibility)
- Updated Presets for newer devices and better quality
- AC3 encoding support.
- Many Bug fixes and other small improvements
- Improved DVD Main Feature detection (when using dvdnav)
- Universal audio downmix support (all audio types can be downmixed)

### All GUIs

- Updated x264 Advanced Panel
- Video Quality Slider drops % value and only shows RF for x264
- Batch Scan (Scan Multiple files at once. N.B: Does not include multiple VIDEO_TS folders / Image files)
- Peak framerate option (Capped VFR)
- Many Bug fixes
- Many Tweaks to improve usability.
- Ability to edit queue jobs
- Point-to-Point encoding (second or frame start and end times)

### Mac GUI

- New Audio Panel supporting >4 Audio Tracks
- VLC detection in /Applications and ~/Applications

### Windows GUI

- Encode Status in GUI. (CLI window is now always hidden)
- Improved Auto-Naming for Destination file name.
- Drag / Drop Video onto Main Window to scan.

### Linux GUI

- Multiple instance support (run multiple copies of ghb at once)
- Many Bug fixes and UI improvements.

## HandBrake 0.9.4

### Core

- New build system, allowing 64-bit binaries (around 10% faster)
- Soft subtitles and Closed Captions:
  - DVD Closed Captions
  - ATSC Closed Captions
  - SRT subtitle import
  - Text soft subtitles in MP4 and MKV output
  - Bitmap soft subtitles in MKV output
- Better support for DVD inputs:
  - Uses libdvdnav
  - DVD angles support
  - Workaround for libdvdread malloc bomb on invalid PGC entry
  - DVD drive region detection support in Linux
  - Handles DVD programs with more than 16 streams
  - No longer tries to detect and discard duplicate titles when scanning
  - Libdvdnav patched to perform read error recovery
  - Libdvdread patched to allow raw device access in Windows
  - Handles poorly mastered DVDs that had the menus ripped out of them
- Better support for non-DVD inputs:
  - Preserves MP4 metadata
  - TrueHD
  - DTS-HD demuxing
  - 8 bit audio
  - Better handling of transport streams where audio starts first
  - Better handling of transport streams that have been spliced together, leading to duplicate timestamps
  - Better VC-1 frame detection
  - Fixes bug that was causing one sec. of audio to be dropped on many ffmpeg files
  - Looks harder for aspect ratio info from DV sources
  - No longer truncates the last (dummy) chapter
  - Allows specifying field parity for detelecine and decomb
- Better AV sync
- Support for sources with no audio
- DTS passthrough for MKV
- x264 bumped from r1169 to r1347, which means speed optimizations, new default settings (see r2742 commit comment), the magic of macroblock tree rate control (mbtree), a new CRF curve (meaning you will get different, generally lower bitrates at the same RF, with similar quality metrics), and weighted P-Frames (disabled by default for Baseline encodes and the AppleTV preset).
- Better sample interleaving
- Better, optional deinterlacer for decomb (EEDI2)
- New mode structure for the decomb filter
- Variable verbose logging levels
- Fixed timing for first two frames coming out of filters
- Libtheora bumped to 1.1.0
- Improvements to our theora implementation (2 pass encoding + soft target rate control)
- Caters to Theora's insistence on content having mod16 framing dimensions specified
- Flushes LAME encoder's final packets
- Fixed interjob framerate calculation
- Fixed pthreads regression in cygwin
- Tweaks for packaging tools
- Solaris 10 build support

### All interfaces

- Live video preview
- New subtitle tab
- New filters and picture settings inspector
- Custom anamorphic mode
- Updated Sparkle
- Custom number of preview images
- Quality slider now works off actual rate factor/quantizer values instead of percentages
- Partially updated advanced x264 tab
- New built-in presets
- Use libdvdnav by default on DVD sources
- Removed Constant QP encoding option for x264 (use CRF)
- Various bug fixes and UI tweaks
- x264 turbo 1st pass mode now uses subme=2 not subme=1

### Mac

- Core Audio AAC encoding
- H.264 video source decoding crash fixed
- Queue displays varying row heights based on encode settings
- Fixed EyeTV package scanning
- 64bit / 32 bit VLC detection
- Preset import/export

### Windows

- New audio tab
- AAC audio source decoding bug fixed
- Tray minimization is now optional
- Queue can now be started from main window
- Growl for Windows notification support
- General UI improvements
- Preset import
- Preferred language control for audio dubs and subtitles
- Fixed file extensions resetting to m4v when enabling chapter markers in mkv
- Faster updating of GUI elements from CLI data
- Cleanup / Improved some of the programs options. (Growl, use m4v, drive detection)
- Numerous fixes in the Picture Settings Panel and CLI Query Handling code.
- Bug Fixes and Usability improvements.

### Linux

- General UI improvements
- Inhibits sleep mode while encoding
- Single title scan
- Chapter duration display
- Notifications when encodes complete
- Tray minimization
- Full screen preview
- Preset import/export
- Preferred language control for audio dubs and subtitles
- Preferences rearrangement
- Preference to auto-apply .m4v extension
- New system tray icon behavior
- Preference for what to do when encode completes
- Preference for how often to delete activity logs
- Preference to disable automatic scanning
- New Gnome session manager support
- Improved "auto" audio selection
- Use .m4v as the default extension for the MPEG-4 container
- Use .m4v when soft subs are enabled
- Alternate angle encoding fix
- Only strips drive letters for Windows builds
- Show correct audio format info when it's been sanitized for incompatibilities
- Preserve chapter list modifications made to queued jobs
- Fixed error when navigating chapter titles with the keyboard
- Bug Fixes.

### CLI

- Options to handle new subtitle, anamorphic, and preview features
- --srt-file, --srt-codeset, --srt-offset, --srt-lang, --srt-default
- --native-dub option lets users request dubbing instead of subs when the audio isn't in their native language
- Allow encoding sources with no audio without explicitly stating -a none
- Update checker on MinGW built exe should now work correctly.
- Matches GUIs' default verbosity level of 1

## HandBrake 0.9.3

- Better PMT processing
- Basic underlying support for live previews (encode from a seek point for a set number of frames)
- Better searching for IDR frames in H.264 streams
- Preset changes (iPhone goes CRF, some old Apple presets resurrected as Legacy)
- Assorted bug fixes

## HandBrake 0.9.3 Snapshot 5 (SVN revision 1913)

### Core Library

- VC-1 stream input
- Newer libmp4v2, which fixes the issue with output > 2 gigs in Linux
- Proper allocation for preview frames
- Avoids corruption of previews of sources that use widths that aren't cleanly divisible by 8
- Decodes DTS internally instead of using ffmpeg, to allow mixdowns
- Better support for DTS in MKV files with implicit timestamps or large timestamp errors
- Ensures proper chroma size by rounding up when dealing with odd dimensions
- Ensures "auto" samplerate sends a valid value to faac (22050, 24000, 32000, 44100, or 48000)
- Bumped Theora to 1.0 final
- Bumped x264 to r1024, which includes Nehalem optimizations as well as speed boosts for things such as b-adapt=2

### Mac GUI

- Allows multibyte characters in chapter titles

### Windows GUI

- Fixes issue parsing presets that use maxWidth and maxHeight (-X and -Y)
- DRC defaults to 1 now
- Chapter markers disabled for non-DVD sources
- Makes sure Normal preset gets loaded
- Fixes arithmetic overflow crash when scanning

### Linux GUI

- Update checker
- Limits range of chapters to encode to the number of chapters on the DVD
- Disabled entry of dimensions larger than the source

### CLI

- Allows overriding of audio (tracks, bitrates, samplerates, codecs, mixdowns) and x264 options in built-in presets

### Documentation

- Documentation updates have begun on the wiki, although they are not yet complete

## HandBrake 0.9.3 Snapshot 4 (SVN revision 1896)

### Core Library

- Converts video in other color spaces to YUV420 (this means DV support)
- Official, standards-based AC3-in-MP4
- Tries to base the AV timing for streams off audio when possible
- Keeps some audio fixes for lost packets in over the air streams from interfering with other sources
- Handles rendering of sources where the picture resolution changes mid-stream (this fixes the long-standing bug reading a particular episode of Doctor Who)
- Wider window for clock references (AV sync)
- Fixed a crash when closing out data for AAC encoding on aborted encodes
- Rejiggered verbose activity log display to be more laconic by default
- Updated x264 to r1016, which means b-rdo and bime are gone and replaced with new subme modes
- DTS and HDMV DTS audio support in streams
- Doesn't show the audio track button on iPhones/iPod Touches unless there's more than 1 track
- Tries to avoid garbage data from AC3 sync by searching for two agreeing packets
- As the MPEG4IP project is defunct, switched to an independently maintained libmp4v2 which has folded in all our cumbersome patches
- Fixed SunOS compilation
- Fixed conflict between maxHeight and maxWidth and loose anamorphic
- Warn in the log when titles are being ignored during scan for lack of audio
- Fixed bug with Slow/Slowest deinterlacing and decomb which could leave a flickering line at the top or bottom of the screen
- Extracts audio and subtitle types from DVD sources, to do things like label commentary tracks
- Better handling of the beginning of AVI and WMV sources that start after time 0
- Optimize MP4 for web download works with AC3 tracks now

### Mac GUI

- Nested presets
- Individual activity logs for each encode (stored by default in ~/Application Support/HandBrake, can be co-located with encoded file destination by preference)
- Allows reading from ZFS volumes
- Fixed target size mode. It keeps breaking itself. Maybe it should just be put out of its misery...
- Assorted other improvements

### Windows GUI

- Nested presets
- Individual activity logs for each encode
- Slow and slower deinterlacing and decomb work now in Windows
- Added resizable update window
- Fixed parsing of non-DVD source audio formats
- Restored Copy to Clipboard to the Activity Log Window, among other enhancements to it
- Fixed bug with MKV presets showing up as .m4v
- Assorted other improvements

### Linux GUI (GTK)

- Nested presets
- Individual activity logs for each encode
- Allows pending queue items to be removed, and reloaded in the main window for editing
- Better handling of HD previews
- Assorted other improvements

### CLI

- Updated presets to the equivalent of the nested ones in the GUIs
- Allows setting custom audio track names in MP4 files
- Allows selection of the COLR atom in MP4 files, between Bt.601 and Bt.709
- Fixed reading of device paths in OS X

### A special note on the new presets (they're collapsible-triangle-folder-thing-errific!)

- Deux Six Quatre, Blind, Broke, and Bedlam are gone. They were dead weight.
- iPod Low-Rez is now iPod Classic & iPod Nano
- iPod High-Rez is now iPod Legacy
- iPhone / iPod Touch is now iPhone & iPod Touch, so take care CLI users
- Animation and Television now use the decomb and detelecine (VFR) filters
- High Profile presets now use psy-trellis and the new subme 9 mode with B-frame RD refinement
- AppleTV is now CRF, so sizes will vary with content
- PS3 preset should be fixed
- Constant Quality Rate still needs its quality % lowered, probably

The keen reader is already asking "iPod Legacy? WTF is iPod High-Rez called iPod Legacy now?"

The answer is Universal.

The Universal preset is designed to play on all modern iPods (anything newer than the iPod 5.5G). It also plays on iPhones. It also plays on AppleTVs. It should also play just about anywhere else, hence the name. It is full anamorphic DVD resolution--no tricks with downscaling like stuff from the iTunes Store. It includes chapters, and has the first audio track in both AAC (DPL2 downmixed) and AC3 pass-thru, just like the AppleTV preset. In fact, it should give the same quality as the AppleTV preset, but faster...and at a larger file size. Like the AppleTV preset, it used CRF, so sizes will vary.

## HandBrake 0.9.3 Snapshot 3 (SVN revision 1797)

### Core Library

- Universal input support, utilizing libavcodec from the FFmpeg project for decoding non-MPEG-2 video
- Newer, faster, better version of the x264 codec, including psychovisual optimizations
- Better AV sync through full compliance with the MPEG Standard Target Decoder timing model
- More accurate auto-cropping
- Support for New Zealand and Norwegian HDTV broadcasts (H.264 and AAC-LATM in MPEG-TS)
- Detelecine is now "VFR detelecine" by default, dropping some frames and extending others to make up lost time, old behavior of keeping duplicate frames is enabled by selecting a framerate besides "Same as source"
- Threaded deinterlacing in Slow and Slower modes
- Threaded and entirely rewritten decomb filter
- Better audio resampling interpolator
- Better gamma in QuickTime through the use of the COLR MP4 atom
- Better constant quality encoding when using FFmpeg
- Hopefully better cache and virtual memory performance by recycling buffers that were most recently used instead of least
- Fix for MP4s with "negative duration" errors.
- Set the detelecine filter to work better with PAL by using "loose" breaks
- Fix for missing initial H.264 NAL units, improves reliability of 8x8dct
- Fix for subtitle-scan with XviD encoding
- Fix for crash at the end of 2nd pass using x264
- Deblock filter works now
- Rewritten update system, so the core library can read a portion of Sparkle appcasts.
- Updates for libsamplerate, libogg, xvidcore, libtheora, libmpeg2, lame, faac, and of course ffmpeg and x264.

### Mac GUI

- Entirely rewritten and far more flexible queue that can be saved between sessions, capable of preserving queued items after a crash
- Now requires vlc 0.9.xx to read protected dvd's in the users /Applications folder
- Fix for 4x3 loose anamorphic to keep it from downscaling
- Countless other improvements

### Windows GUI

- Resolution calculation
- Better preset bar
- Better queue (including queue recovery feature)
- Better activity log window
- Improved UI (layout changes, animated x264 options, DVD drive detection, duration displayed)
- More options - includes support for custom auto name format & starting the CLI minimized
- Countless other improvements

### Linux GUI (GTK)

- It's alive!

### Known Issues in Snapshot 3

- Possibility of a flickering line at the top or bottom of the frame after Slow or Slower deinterlacing or decombing
- Input bitrate display may be off by a factor of 100 for H.264-in-TS sources
- Constant Quality Rate preset probably needs a lower quality level (60% - 55%)
- With non-DVD sources that don't have AC3 audio, you can't encode 1 input audio track to multiple output audio tracks
- Slow and Slower deinterlacing and decombing are BROKEN in Windows
- QuickTime won't read Xvid-in-MP4 output, although VLC will
- Windows GUI does not detect all audio tracks from non-DVD sources

## HandBrake 0.9.3 Snapshot 2 (SVN revision 1477)

### Core Library

- Anamorphic PAR for the AVI container
- Allow constant frame rates when they different from the source's frame rate (otherwise pass through the variable MPEG-2 frame durations )
- Decomb filter (selectively deinterlaces when it sees interlacing in the frame)
- Filter bug fixed, that would skip any filters after detelecine, if VFR wasn't enabled
- Loose anamorphic + FFmpeg video bug fixed

### Windows GUI

- Title dropdown list bug fixed
- Missing log file bug fixed

### CLI

- Default audio samplerate changed to 48kHz, audio bitrate changed to 160kbps.
- Samplerate entry bug fixed

## HandBrake Snapshot 1 (SVN revision 1457)

### Core Library

- New audio subsystem (no more AAC+AC3, control each track's codec and settings individually)
- Removed libdvdcss (HandBrake no longer decrypts DVDs on its own, will use VLC to do so if it's available)
- Added Theora encoder
- Fixed x264-in-avi and ffmpeg-in-avi
- Fixed xvid
- More accurate scaling
- Major sync improvements
- Major stream improvements
- AAC+AC3 support in MKV
- MKV seeking fixes
- Make sure subtitles get displayed long enough to read them
- Updated VBV 2-pass and VBV 1-pass patch for x264
- Adaptive Quantization for x264
- Recover from bad preview scans
- Recover from invalid PGNs
- Fixed vorbis bitrate control
- Snapshot builds

### Mac

- New audio interface
- Loads libdvdcss from VLC at runtime if it's present on the user's system
- No more general-purpose "Codecs" menu -- set video and audio codecs individually
- More robust preset system, in preparation for nested presets
- Made 64-bit MP4 file widget more prominent
- Only allow useful x264 options in the advanced tab
- Various fixes and improvements

### Windows

- New x264 tab
- New audio interface
- Various fixes and improvements

### CLI

- New audio interface

## HandBrake 0.9.2

### Core

- AC3 in MP4 support
- Multi-track audio support for Apple devices
- Better handling of audio discontinuities
- More flexible, "loose" anamorphic
- Variable frame rate encoding
- MP4 optimization for progressive downloads
- Dynamic range compression for encoding from AC3 audio
- Ability to encode an audio stream and pass it through at the same time
- iPhone-compatible anamorphic (pasp atom)
- Robust program and transport stream support
- Better handling of DVD read errors from invalid VOB units
- Detects and works around missing end of cell markers
- Recovers from loss of signal in a stream
- Drops subtitles less often
- Keeps chapter markers in better sync and prevents duplicates
- Better handling of B-Frames
- Tunes FIFO sizes by CPU count
- Finally squashes the bug that cut off the end of movies
- Preset changes
- Standardizes on standard out for progress and standard error for everything else.
- Correct channel counts when passing AC3 audio to Matroska
- Tag MP4 files as encoded with HandBrake
- No more merging short chapters
- Newer copies of x264,
- VBV 2-pass patch for x264
- Sets keyframes for x264 by frame rate.
- Support for >2GB MKV files in Linux
- Code audio languages in a way QuickTime understands
- Better subtitle positioning
- Fewer crashes in 2-pass encoding

### Mac

- Leopard Only
- Sparkle
- Reads .eyetv files as well as .dvdmedia files
- Much better queue
- More white space
- Code restructuring
- Activity window logging, complete with a "black box recorder" for crashes
- Ability to open a single title for a DVD instead of scanning the whole thing
- Warns people when they try to queue up two files with the same name
- Maintains picture filter states between jobs
- .xib Interface Builder files SVN can track
- Switches to NSImageView for previews, so no more useless OpenGL effects
- Temporary loss of localizations for foreign languages (the old system was broken anyway)
- Separate filter settings for every queued job

### Win

- Revamped preset system
- Sparkle-compatible update checker
- Activity log window
- CLI built-in preset parsing
- No more admin rights required in Vista
- Handles more display resolutions

### CLI

- Built-in presets
- Short names for denoising (weak, medium, strong) and deinterlacing (fast, slow, slower)
- Solaris port
- No more x264b30 (use -e x264 -I -x level=30:cabac=0 instead or better yet an iPod preset)
- Chapter marker .csv input fixed
- CRF as default quality mode for x264, now -q is CRF and if you want CQP add -Q to it

## HandBrake 0.9.1

### Core HandBrake Changes

- Added: Forced subtitle support
- Added: 6-channel Vorbis audio
- Changed: Much better buffer management, leading to impressive speed-ups all over the place
- Changed: Color subtitles now display in color, instead of being transparent.
- Changed: All errors to stderr with hb_log() instead of to stdout with fprintf()
- Changed: Accept stream input where the file type is in caps (.VOB instead of just .vob, etc)
- Changed: Better quality Vorbis codec (AoTuV)
- Changed: Faster (threaded) ffmpeg
- Changed: Force x264 to use a key frame at chapter markers
- Changed: Try to recover from bad preview scans instead of crashing
- Fixed: No more hanging when using MKV with chapter markers
- Fixed: "Same as source" FPS now works correctly when the end-credits of a progressive film are interlaced.
- Fixed: "Slow" deinterlacing no longer doubles up the chapter markers
- Fixed: Proper display of fading subtitles
- Fixed: Nasty artifacts from inaccurate rounding in the video scaler
- Fixed: Improved compatibility with streams that have missing/misplaced PMTs
- Assorted other changes

### Mac Changes

- Changed: Bigger buffer for the Activity Log
- Changed: Redesigned Queueing window
- Changed: Redesigned Preferences window
- Changed: Structural reorganization of the code into more segmented files
- Fixed: Closing the main window no longer causes HandBrake to quit
- Fixed: Changing dimensions in Picture Settings no longer causes a crash
- Fixed: Target size bitrate calculation
- Fixed: Picture Settings previews now scale to display resolution and screen size
- Assorted other changes

### Windows Changes

- Added: More robust exception handling
- Added: On-completion options to shutdown, suspend, etc
- Added: Turn tooltips on or off
- Changed: Open source, NullSoft installer
- Fixed: Add-to-queue issues
- Fixed: Foreign language issues
- Assorted other changes

## HandBrake 0.9.0

### Core HandBrake Changes

- Added: Matroska (MKV) container output
- Added: Limited MPEG-2 transport stream (.VOB and .TS) input support
- Added: Option to write MP4 files larger than 4GB
- Added: Video filters (pullup, yadif, mcdeint, hqdn3d, pp7)
- Added: DTS audio input
- Changed: Switched to Lanczos scaling from libswscale
- Changed: Precise chapter marker location
- Changed: Newer libraries
- Changed: Much faster (threaded) iPod encoding
- Changed: "Same as source" works differently (better?) now
- Fixed: Audio drops should be thoroughly banished now
- Fixed: MP2 audio support
- Assorted other changes

### CLI Changes

- Added: Chapter naming
- Added: Many new command line options for subtitles and filters.
- Added: Turbo for 2-pass x264 encodes
- Assorted other changes

### Mac Changes

- Added: Chapter naming
- Added: Growl support
- Added: Advanced x264 settings tab
- Added: Logging window
- Added: Turbo for 2-pass x264 encodes
- Added: Many new presets
- Added: Unified toolbar
- Changed: Default settings
- Changed: Further integration of the queue and active queuing
- Changed: Browse DVDs like any other volumes
- Fixed: No more floating window syndrome (Mac)
- Fixed: Presets retain "magic sauce" when you change settings
- Assorted other changes

### Windows Changes

- Changed: New C#-based Windows GUI front-end
- Changed: Improved queuing
- Changed: DVD information parser
- Assorted other changes

## HandBrake 0.8.5b1

### Core HandBrake Changes

- Added: iTunes-style chapter markers.
- Added: 5.1 AAC surround sound.
- Added: Dolby Pro Logic I and II downmixing of discrete surround sound.
- Added: 1-channel AAC sound from monophonic sources.
- Added: Advanced x264 options. (including High Profile support)
- Added: B-frames in x264 + .mp4
- Added: PPC Linux Support.
- Added: Preserve language IDs from the DVD in .mp4
- Added: Snapshot build method.
- Added: Anamorphic video display in QuickTime.
- Changed: Renamed back to HandBrake.
- Changed: Libraries updated.
- Changed: Enabled Update Checker.
- Fixed: Multiple Audio tracks.
- Fixed: Sped up DVD scanning time by being nicer to libdvdread.
- Fixed: .dmg is now mountable in Mac OS X versions older than 10.4
- Fixed: Proper output size from x264 in target size mode.
- Fixed: Allows output sizes larger than 2 gigs in Linux.
- Fixed: Several small memory leaks have been plugged.
- Fixed: Fixes for 64-bit systems.
- Fixed: Keep Aspect Ratio is no longer forced, so user-set height values are respected.

### CLI Interface Changes

- Added: Customize maximum width and height while keeping aspect ratio
- Changed: Much prettier help screen
- Changed: HBTest/MediaForkCLI renamed to HandBrakeCLI
- Fixed: Better display of audio and subtitle ids

### Mac GUI Changes

- Added: Presets! Includes initial ones for AppleTV, iPod, and PS3.
- Added: Preference option to auto-name output files with the DVD name and title number.
- Added: Preset support for x264 options.
- Changed: Remembers last destination path.
- Changed: Remembers last source path.
- Changed: Copy and paste in text fields.
- Changed: Updates target size more quickly.
- Changed: Mac GUI no longer retains target size values between enqueued jobs. (http://HandBrake.m0k.org/forum/viewtopic.php?t=249)
- Fixed: Preview frames are no longer distorted in anamorphic mode.
- Fixed: Mac GUI no longer floats above other windows.
- Fixed: Browse by file no longer dims the browse button preventing you from changing browse locations without switching back and forth between it and drive selection. (http://HandBrake.m0k.org/forum/viewtopic.php?t=342)
- Fixed: Makes sure destination directory is valid.
- Fixed: Fills in the file save field with the current output name instead of leaving it blank.
- Fixed: Update destination field with the current path instead of using the last one, which could have been a DVD.

### Windows GUI Changes - Version 2.2 beta 1

- Added: A few presets for the iPod in the menu.
- Added: Ability to set default settings for all program encode options.
- Added: Ability to turn off Automatic Update check on start-up. See Tools > Options
- Added: Mod 16 check on the Height and Width boxes.
- Added: Check the amount of hard disk space left is not running low when saving file.
- Added: Option to have a Read DVD window showup on start-up.
- Added: ìView DVD dataî Menu item in the tools menu.
- Added: Links to the Homepage, forum, wiki and documentation page in the Help menu.
- Added: Chapter markers check box (New feature in 0.8.5b1 CLI)
- Changed: View DVD Information no longer appears after clicking the ìBrowseî button.
- Changed: A few changes to the GUI - replaced textboxes with Dropdowns which auto-populate.
- Changed: Auto Crop and Aspect text now automatically update when a new title is selected.
- Changed: Several tweaks to the GUI design, remove a few text items that are no longer needed.
- Changed: Ability to Queue videos enabled with completely re-written code.
- Changed: Ability to queue stuff up while the encoding process is running.
- Changed: Ability to remove items from the encode queue while is running.
- Changed: Anamorphic option blanks out resolution boxes.
- Changed: Re-written update checker.
- Changed: Ability to turn off update check on start-up in Tools > Options
- Changed: Auto Crop option now fills in figures into text boxes when selected.
- Changed: Mp4 now default output file extension.
- Changed: Enabled 5.1 AAC option.
- Changed: Enabled h264 advanced options.
- Changed: Updated the FAQ.
- Changed: Included new version of HandBrake. Version 0.8.5b1.
- Fixed: Pixel Ratio Not being saved with the profile.
- Removed: Both ìView Dataî buttons on the Title Selection Window.
- Removed: The ìRead DVDî button. -  Automatically reads the DVD after selecting a source now.
- Removed: The Help and Support window. Been replaced with a few Web Links.

## HandBrake 0.8.0

- MediaFork project forked from HandBrake source <http://handbrake.m0k.org/>
- Updated libraries (meaning better quality, hopefully fewer bugs, and increased speeds)
- iPod 5.5G support
- Revamped graphical interface (Mac OS X)
- Anamorphic encoding with pixel aspect ratio
- Brighter color reproduction in QuickTime
- Lists disks by DVD name instead of by drive name (Mac OS X)
- Titles output movies based on the DVD name (Mac OS X)
- 32Khz audio output
- Constant rate factor encoding with x264
- New preference item to turn deinterlacing on by default (Mac OS X)
- New preference item to select the default audio language (Mac OS X)
- Bugfix for reading straight from a DVD

## HandBrake 0.7.1

- Universal Binary for PPC and Intel
- Bugfixes for missing subtitles, audio glitches with LPCM tracks and more

## HandBrake 0.7.0

- Multithreaded H.264 encoding with x264
- Added option for H.264 Baseline (suitable for iPods)
- (Very) experimental queue support
- Fixes for some DVD titles HandBrake would not recognize
- Fixes audio glitches when encoding from LPCM tracks

## HandBrake 0.7.0-beta3

- Chapters selection
- Custom framerate
- Subtitle support
- Check for updates
- Custom aspect ratio
- Audio samplerate selection
- mp4/H.264 output
- Proper NTSC support
- AC3 pass-through
- Progress bar in the dock icon (OS X)
- 2-pass H.264 encoding
- Constant quality encoding
- Grayscale encoding
- Up-to-date BeOS UI

## HandBrake 0.6.2

- Support for DVDs with MPEG audio tracks
- Rewrote the DVD navigation code
- High quality resampler included
- Better AVI compliance
- Updated encoders
- Internal improvements
- Bugfixes

## HandBrake 0.6.1

- Fixed LPCM endianness issue

## HandBrake 0.6.0

- MP4 and OGM output
- AAC and Vorbis encoding
- Experimental H264 encoding
- LPCM DVDs support
- Autocrop
- GTK2 linux interface
- OS X interface localization

## HandBrake 0.5.2

- Bugfixes

## HandBrake 0.5.1

- 2-pass XviD encoding
- Bugfixes

## HandBrake 0.5

- Bugfixes, rewrite of large parts of the core
- XviD encoding (1-pass only)

## HandBrake 0.4

- Better multithreading
- Allow the user to specify a target size instead of bitrate
- Misc GUI enhancements
- Use low-priority threads on OS X

## HandBrake 0.3

- OSX & Linux ports
- Allow 2-pass encoding
- Many internal changes & fixes

## HandBrake 0.2

- Fixed a major bug that made HandBrake probably crash after ~ 15
   minutes encoded
- Fixed a few minor memory leaks

## HandBrake 0.1.1

- Fixed a stupid bug that prevented to scan volumes correctly if FAT/NTFS/etc volumes were mounted

## HandBrake 0.1

- Automatically detect ripped DVDs on BFS volumes
- Allow picture cropping and resizing
- Allow dual-audio encoding
- Created files are quite compliant now (tested with OSX/Quicktime and BSPlayer)
- Better A/V sync with some DVDs

## HandBrake 0.1-alpha2

- Show length for each title
- Fixed the screwed-audio bug
- Many bugfixes...

## HandBrake 0.1-alpha

- First version.

