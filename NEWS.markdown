# HandBrake News

## HandBrake 1.0.3

### All platforms

#### Video 
- Fixed a couple of potential crashes whilst attempting to read source files.

### Linux
- Fixed an issue with HQND3d Denoise filter custom text entry box rejecting values

### Mac
- Fixed an issue with Core AAC audio encoder not starting at the correct start point
- Fixed an crash when attempting to edit a queued job

### Windows
- Fixed an issue that could cause error -17 when encoding with Intel QuikcSync
- Fixed an issue with the SRT language code not being set correctly
- Fixed missing 'Tape' and 'Sprite' Denoise Tunes


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
- Added resizeable update window
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
- Fixes audio gliches when encoding from LPCM tracks

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
