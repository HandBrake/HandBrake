#! /usr/bin/ruby
# manincure.rb version 0.66

# This file is part of the HandBrake source code.
# Homepage: <http://handbrake.m0k.org/>.
# It may be used under the terms of the GNU General Public License.

# This script parses HandBrake's Mac presets into hashes, which can
# be displayed in various formats for use by the CLI and its wrappers.

# For handling command line arguments to the script
require 'optparse'
require 'ostruct'
require 'rubygems'
require 'plist'

# CLI options: (code based on http://www.ruby-doc.org/stdlib/libdoc/optparse/rdoc/index.html )
def readOptions
  
  # --[no-]cli-raw, -r gives raw CLI for wiki
  # --cli-parse, -p gives CLI strings for wrappers
  # --api, -a gives preset code for test.c
  # --api-list, -A gives CLI strings for --preset-list display
  # --[no-]header, -h turns off banner display
  options = OpenStruct.new
  options.cliraw = false
  options.cliparse = false
  options.api = false
  options.apilist = false
  options.header = false
  
  opts = OptionParser.new do |opts|
    opts.banner = "Usage: manicure.rb [options]"
    
    opts.separator ""
    opts.separator "Options:"
    
    opts.on("-r", "--cli-raw", "Gives example strings for the HB wiki") do |raw|
      options.cliraw = raw
      option_set = true
    end
    
    opts.on("-p", "--cli-parse", "Gives presets as wrapper-parseable CLI", " option strings") do |par|
      options.cliparse = par
    end
    
    opts.on("-a", "--api", "Gives preset code for test.c") do |api|
      options.api = api
    end
    
    opts.on("-A", "--api-list", "Gives code for test.c's --preset-list", " options") do |alist|
      options.apilist = alist
    end
    
    opts.on("-H", "--Header", "Display a banner before each preset") do |head|
      options.header = head
    end
    
    opts.on_tail("-h", "--help", "Show this message") do
        puts opts
        exit
    end
  end.parse!
  
  return options
  
end

# These arrays contain all the other presets and hashes that are going to be used.
# Yeah, they're global variables. In an object-oriented scripting language.
# Real smooth, huh?

# This class parses the user's presets .plist into an array of hashes
class Presets
  
  attr_reader :hashMasterList
  
  # Running initialization runs everything.
  # Calling it will also call the parser
  # and display output.
  def initialize
    
   # Grab the user's home path
   homeLocation = `echo $HOME`.chomp
   
   # Use that to build a path to the presets .plist
   inputFile = homeLocation+'/Library/Application Support/HandBrake/UserPresets.plist'
   
    # Parse the presets into hashes
    @hashMasterList = Plist::parse_xml( inputFile )
    
  end

end

# This class displays the presets to stdout in various formats.
class Display
  
  def initialize(hashMasterList, options)
  
    @hashMasterList = hashMasterList
    @options = options

    # A width of 40 gives nice, compact output.
    @columnWidth=40
    
    # Print to screen.
    displayCommandStrings
    
  end
  
  def displayCommandStrings # prints everything to screen
    
    # Iterate through the hashes.    
    @hashMasterList.each do |hash|
    
      # Check to see whether we've got a preset or afolder
      if !hash["Folder"]
        # It's a top-level preset
       displayIndividualPreset(hash, 0) 
      else
        # It's a folder, yay
        displayFolder( hash, 0 )
        hash["ChildrenArray"].each do |subhash|
          # Drill down to see its contents
          if !subhash["Folder"]
            # It's a preset
            displayIndividualPreset(subhash, 1)
          else
            # It's a folder
            displayFolder( subhash, 1 )
            subhash["ChildrenArray"].each do |subsubhash|
              # At this point we're far enough down we won't try to drill further
              if !subsubhash["Folder"]
                displayIndividualPreset(subsubhash, 2)
              end
            end
            displayFolderCloser( 1 )
          end
        end
        displayFolderCloser( 0 )
      end
    end
  end
  
  def displayIndividualPreset(hash, depth)
    if @options.header == true
      # First throw up a header to make each preset distinct
      displayHeader(hash)
    end
    
    if @options.cliraw == true
      # Show the preset's full CLI string equivalent
      generateCLIString(hash, depth)
    end
    
    if @options.cliparse == true
      generateCLIParse(hash, depth)
    end
    
    if @options.api == true
      # Show the preset as code for test/test.c, HandBrakeCLI
      generateAPIcalls(hash)
    end
    
    if @options.apilist == true
      # Show the preset as print statements, for CLI wrappers to parse.
      generateAPIList(hash, depth) 
    end
  end
  
  def displayHeader(hash) # A distinct banner to separate each preset
    
    # Print a line of asterisks
    puts "*" * @columnWidth
    
    # Print the name, centered
    puts '* '+hash["PresetName"].to_s.center(@columnWidth-4)+' *'
    
    # Print a line of dashes
    puts '~' * @columnWidth
    
    # Print the description, centered and word-wrapped
    puts hash["PresetDescription"].to_s.center(@columnWidth).gsub(/\n/," ").scan(/\S.{0,#{@columnWidth-2}}\S(?=\s|$)|\S+/)
    
    # Print another line of dashes
    puts '~' * @columnWidth
    
    # Print the formats the preset uses
    puts "#{hash["FileCodecs"]}".center(@columnWidth)
    
    # Note if the preset isn't built-in
    if hash["Type"] == 1
      puts "Custom Preset".center(@columnWidth)
    end

    # Note if the preset is marked as default.
    if hash["Default"] == 1
      puts "This is your default preset.".center(@columnWidth)
    end
    
    # End with a line of tildes.  
    puts "~" * @columnWidth
    
  end
  
  def displayFolder( hash, depth )

    if @options.cliraw == true
      # Show the folder's full in a format that matches the CLI equivalents
      generateCLIFolderString(hash, depth)
    end
    
    if @options.cliparse == true
      # Show the folder in a format that matches the CLI wrapper equivalents
      generateCLIFolderParse(hash, depth)
    end
    
    if @options.apilist == true
      # Show the folder as print statements, for CLI wrappers to parse.
      generateAPIFolderList(hash, depth) 
    end
    
  end
  
  def displayFolderCloser( depth )
    if @options.cliraw == true
      # Show the folder's full in a format that matches the CLI equivalents
      generateCLIFolderCloserString( depth )
    end
    
    if @options.cliparse == true
      # Show the folder in a format that matches the CLI wrapper equivalents
      generateCLIFolderCloserParse( depth )
    end
    
    if @options.apilist == true
      # Show the folder as print statements, for CLI wrappers to parse.
      generateAPIFolderCloserList( depth ) 
    end
  end
  
  def generateCLIFolderString( hash, depth ) # Shows the folder for the CLI equivalents
    commandString = ""
    depth.times do
      commandString << "   "
    end
    (depth+1).times do
      commandString << "<"
    end
    commandString << " " << hash["PresetName"] << "\n"
    puts commandString
  end
  
  def generateCLIFolderCloserString( depth )
    commandString = ""
    depth.times do
      commandString << "   "
    end
    (depth+1).times do
      commandString << ">"
    end
    commandString << "\n"
    puts commandString
  end
  
  def generateCLIString(hash, depth) # Makes a full CLI equivalent of a preset
    commandString = ""
    depth.times do
      commandString << "   "
    end
    commandString << './HandBrakeCLI -i DVD -o ~/Movies/movie.'
    
    #Filename suffix
    case hash["FileFormat"]
    when /MPEG-4/, /MP4/
      commandString << "mp4 "
    when /Matroska/, /MKV/
      commandString << "mkv "
    end
    
    #Video encoder
    commandString << " -e "
    case hash["VideoEncoder"]
    when /x264/
      commandString << "x264"
    when /Theora/
      commandString << "theora"
    when /MPEG-4/
      commandString << "ffmpeg4"
    when /MPEG-2/
      commandString << "ffmpeg2"
    end

    #VideoRateControl
    case hash["VideoQualityType"]
    when 0
      commandString << " -S " << hash["VideoTargetSize"]
    when 1
      commandString << " -b " << hash["VideoAvgBitrate"]
    when 2
      commandString << " -q " << hash["VideoQualitySlider"].to_s
    end

    #FPS
    if hash["VideoFramerate"] != "Same as source"
      if hash["VideoFramerate"] == "23.976 (NTSC Film)"
        commandString << " -r " << "23.976"
      elsif hash["VideoFramerate"] == "29.97 (NTSC Video)"
        commandString << " -r " << "29.97"
      elsif hash["VideoFramerate"] == "25 (PAL Film/Video)"
        commandString << " -r " << "25"
      else
        commandString << " -r " << hash["VideoFramerate"]
      end
      # not same as source: pfr, else default (cfr)
      if hash["VideoFramerateMode"] == "pfr"
        commandString << " --pfr "
      end
    # same as source: cfr, else default (vfr)
    elsif hash["VideoFramerateMode"] == "cfr"
      commandString << " --cfr "
    end
    
    #Audio tracks
    audioBitrates = ""
    audioEncoders = ""
    audioMixdowns = ""
    audioSamplerates = ""
    audioTracks = ""
    audioTrackDRCs = ""
    audioCount = hash["AudioList"].size
    
    hash["AudioList"].each do |audioTrack|
      audioCount = audioCount - 1

      #Bitrates
      audioBitrates << audioTrack["AudioBitrate"]
      
      #Encoders
      case audioTrack["AudioEncoder"]
        when /AC3 Pass/
          audioEncoders << "copy:ac3"
        when /AC3/
          audioEncoders << "ac3"
        when /DTS Pass/
          audioEncoders << "copy:dts"
        when /DTS-HD Pass/
          audioEncoders << "copy:dtshd"
        when /AAC Pass/
          audioEncoders << "copy:aac"
        when "AAC (ffmpeg)", "AAC (avcodec)", "AAC (faac)"
          audioEncoders << "av_aac"
        when "AAC (FDK)"
          audioEncoders << "fdk_aac"
        when "HE-AAC (FDK)"
          audioEncoders << "fdk_haac"
        when "AAC (CoreAudio)"
          audioEncoders << "ca_aac"
        when "HE-AAC (CoreAudio)"
          audioEncoders << "ca_haac"
        when /Vorbis/
          audioEncoders << "vorbis"
        when /MP3 Pass/
          audioEncoders << "copy:mp3"
        when /MP3/
          audioEncoders << "mp3"
        when "FLAC (ffmpeg)", "FLAC 16-bit"
          audioEncoders << "flac16"
        when "FLAC (24-bit)", "FLAC 24-bit"
          audioEncoders << "flac24"
        when /Auto Pass/
          audioEncoders << "copy"
      end
      
      #Mixdowns
      case audioTrack["AudioMixdown"]
        when "Mono (Left Only)"
          audioMixdowns << "left_only"
        when "Mono (Right Only)"
          audioMixdowns << "right_only"
        when /Mono/
          audioMixdowns << "mono"
        when /Stereo/
          audioMixdowns << "stereo"
        when /Dolby Surround/
          audioMixdowns << "dpl1"
        when /Dolby Pro Logic II/
          audioMixdowns << "dpl2"
        when /5.1/, /discrete/
          audioMixdowns << "5point1"
        when /6.1/
          audioMixdowns << "6point1"
        when "7.1 (5F/2R/LFE)"
          audioMixdowns << "5_2_lfe"
        when /7.1/
          audioMixdowns << "7point1"
        when /None/
          audioMixdowns << "none"
      end
      
      #Samplerates
      audioSamplerates << audioTrack["AudioSamplerate"]
      
      #Tracks
      audioTracks << audioTrack["AudioTrack"].to_s
      
      #DRC
      audioTrackDRCs << audioTrack["AudioTrackDRCSlider"].to_s
      
      if audioCount > 0
        audioBitrates << ","
        audioEncoders << ","
        audioMixdowns << ","
        audioSamplerates << ","
        audioTracks << ","
        audioTrackDRCs << ","
      end
      
    end
    commandString << " -a " << audioTracks
    commandString << " -E " << audioEncoders
    commandString << " -B " << audioBitrates
    commandString << " -6 " << audioMixdowns
    commandString << " -R " << audioSamplerates
    commandString << " -D " << audioTrackDRCs
    
    #Auto Passthru Mask
    audioCopyMask = ""
    
    if hash["AudioAllowAACPass"].to_i == 1
      audioCopyMask << "aac"
    end
    if hash["AudioAllowAC3Pass"].to_i == 1
      if audioCopyMask.size > 0
        audioCopyMask << ","
      end
      audioCopyMask << "ac3"
    end
    if hash["AudioAllowDTSHDPass"].to_i == 1
      if audioCopyMask.size > 0
        audioCopyMask << ","
      end
      audioCopyMask << "dtshd"
    end
    if hash["AudioAllowDTSPass"].to_i == 1
      if audioCopyMask.size > 0
        audioCopyMask << ","
      end
      audioCopyMask << "dts"
    end
    if hash["AudioAllowMP3Pass"].to_i == 1
      if audioCopyMask.size > 0
        audioCopyMask << ","
      end
      audioCopyMask << "mp3"
    end
    
    if audioCopyMask.size > 0
      commandString << " --audio-copy-mask " << audioCopyMask
    end
    
    #Auto Passthru Fallback
    audioEncoderFallback = ""
    
    case hash["AudioEncoderFallback"]
      when /AC3/
        audioEncoderFallback << "ac3"
      when "AAC (ffmpeg)", "AAC (avcodec)", "AAC (faac)"
        audioEncoderFallback << "av_aac"
      when "AAC (FDK)"
        audioEncoderFallback << "fdk_aac"
      when "HE-AAC (FDK)"
        audioEncoderFallback << "fdk_haac"
      when "AAC (CoreAudio)"
        audioEncoderFallback << "ca_aac"
      when "HE-AAC (CoreAudio)"
        audioEncoderFallback << "ca_haac"
      when /Vorbis/
        audioEncoderFallback << "vorbis"
      when /MP3/
        audioEncoderFallback << "mp3"
      when "FLAC (ffmpeg)", "FLAC 16-bit"
        audioEncoderFallback << "flac16"
      when "FLAC (24-bit)", "FLAC 24-bit"
        audioEncoderFallback << "flac24"
    end
    
    if audioEncoderFallback.size > 0
      commandString << " --audio-fallback " << audioEncoderFallback
    end
        
    #Container
    commandString << " -f "
    case hash["FileFormat"]
    when "MPEG-4 (mp4v2)"
      commandString << "mp4v2"
    when /MP4/
      commandString << "mp4"
    when "Matroska (libmkv)"
      commandString << "libmkv"
    when /MKV/
      commandString << "mkv"
    end
    
    #iPod MP4 atom
    if hash["Mp4iPodCompatible"].to_i == 1
      commandString << " -I"
    end
    
    #MP4 Optimize for HTTP Streaming
    if hash["Mp4HttpOptimize"].to_i == 1
      commandString << " -O"
    end
    
    #Cropping
    if hash["PictureAutoCrop"] == 0
      commandString << " --crop "
      commandString << hash["PictureTopCrop"].to_s
      commandString << ":"
      commandString << hash["PictureBottomCrop"].to_s
      commandString << ":"
      commandString << hash["PictureLeftCrop"].to_s
      commandString << ":"
      commandString << hash["PictureRightCrop"].to_s
    end
    
    #Dimensions
    if hash["PictureWidth"] != 0
      commandString << " -X "
      commandString << hash["PictureWidth"].to_s
    end
    if hash["PictureHeight"] != 0
      commandString << " -Y "
      commandString << hash["PictureHeight"].to_s
    end
    
    #Subtitles
    if hash["Subtitles"] && hash["Subtitles"] != "None"
      if hash["Subtitles"] == "Autoselect"
        commandString << " --subtitle-scan"
      else
        commandString << " -s "
        commandString << hash["Subtitles"]
      end
    end

    #Video Filters
    if hash["UsesPictureFilters"] == 1
      
      case hash["PictureDeinterlace"]
      when 1
        commandString << " --deinterlace=" << hash["PictureDeinterlaceCustom"].to_s
      when 2
        commandString << " --deinterlace=fast"
      when 3
        commandString << " --deinterlace=slow"
      when 4
        commandString << " --deinterlace=slower"
      when 5
        commandString << " --deinterlace=bob"
      end
      
      case hash["PictureDenoise"]
      when 1
        commandString << " --denoise=" << hash["PictureDenoiseCustom"].to_s
      when 2
        commandString << " --denoise=weak"
      when 3
        commandString << " --denoise=medium"
      when 4
        commandString << " --denoise=strong"
      end
      
      case hash["PictureDecomb"]
      when 1
        commandString << " --decomb=" << hash["PictureDecombCustom"].to_s
      when 2
        commandString << " --decomb"
      when 3
        commandString << " --decomb=fast"
      when 4
        commandString << " --decomb=bob"
      end

      case hash["PictureDetelecine"]
        when 1
          commandString << " --detelecine=" << hash["PictureDetelecineCustom"].to_s
        when 2
          commandString << " --detelecine"
      end

      if hash["PictureDeblock"] != 0
        commandString << " --deblock=" << hash["PictureDeblock"].to_s
      end
      
    end
    
    #Anamorphic
    if hash["PicturePAR"] == 1
      commandString << " --strict-anamorphic"
    elsif hash["PicturePAR"] == 2
      commandString << " --loose-anamorphic"
    elsif hash["PicturePAR"] == 3
      commandString << " --custom-anamorphic"
    end

    #Modulus
    if hash["PictureModulus"]
      commandString << " --modulus " << hash["PictureModulus"].to_s
    end

    #Booleans
    if hash["ChapterMarkers"] == 1 then commandString << " -m" end
    if hash["VideoGrayScale"] == 1 then commandString << " -g" end
    if hash["VideoTwoPass"] == 1 then commandString << " -2" end
    if hash["VideoTurboTwoPass"] == 1 then commandString << " -T" end

    #Advanced Options
    if hash["x264UseAdvancedOptions"] != 1
      if hash["x264Preset"] != ""
        commandString << " --x264-preset "
        commandString << hash["x264Preset"]
      end
      if hash["x264Tune"] != "" && hash["x264Tune"] != "none"
        commandString << " --x264-tune "
        commandString << hash["x264Tune"]
      end
      if hash["h264Profile"] != "" && hash["h264Profile"] != "auto"
        commandString << " --h264-profile "
        commandString << hash["h264Profile"]
      end
      if hash["h264Level"] != "" && hash["h264Level"] != "auto"
        commandString << " --h264-level "
        commandString << hash["h264Level"]
      end
      if hash["x264OptionExtra"] != ""
        commandString << " -x "
        commandString << hash["x264OptionExtra"]
      end
    elsif hash["x264Option"] != ""
      commandString << " -x "
      commandString << hash["x264Option"]
    end
    
    # That's it, print to screen now
    puts commandString
  end
  
  def generateCLIFolderParse( hash, depth ) # Shows the folder for wrappers to parse
    commandString = ""
    depth.times do
      commandString << "   "
    end
    (depth+1).times do
      commandString << "<"
    end
    commandString << " " << hash["PresetName"] << "\n"
    puts commandString
  end
  
  def generateCLIFolderCloserParse( depth )
    commandString = ""
    depth.times do
      commandString << "   "
    end
    (depth+1).times do
      commandString << ">"
    end
    commandString << "\n"
    puts commandString
  end
  
  def generateCLIParse(hash, depth) # Makes a CLI equivalent of all user presets, for wrappers to parse
    commandString = ""
    depth.times do
      commandString << "   "
    end
    commandString << '+ ' << hash["PresetName"] << ":"
        
    #Video encoder
    commandString << " -e "
    case hash["VideoEncoder"]
    when /x264/
      commandString << "x264"
    when /Theora/
      commandString << "theora"
    when /MPEG-4/
      commandString << "ffmpeg4"
    when /MPEG-2/
      commandString << "ffmpeg2"
    end

    #VideoRateControl
    case hash["VideoQualityType"]
    when 0
      commandString << " -S " << hash["VideoTargetSize"]
    when 1
      commandString << " -b " << hash["VideoAvgBitrate"]
    when 2
      commandString << " -q " << hash["VideoQualitySlider"].to_s
    end

    #FPS
    if hash["VideoFramerate"] != "Same as source"
      if hash["VideoFramerate"] == "23.976 (NTSC Film)"
        commandString << " -r " << "23.976"
      elsif hash["VideoFramerate"] == "29.97 (NTSC Video)"
        commandString << " -r " << "29.97"
      elsif hash["VideoFramerate"] == "25 (PAL Film/Video)"
        commandString << " -r " << "25"
      else
        commandString << " -r " << hash["VideoFramerate"]
      end
      # not same as source: pfr, else default (cfr)
      if hash["VideoFramerateMode"] == "pfr"
        commandString << " --pfr "
      end
    # same as source: cfr, else default (vfr)
    elsif hash["VideoFramerateMode"] == "cfr"
      commandString << " --cfr "
    end
    
    #Audio tracks
    audioBitrates = ""
    audioEncoders = ""
    audioMixdowns = ""
    audioSamplerates = ""
    audioTracks = ""
    audioTrackDRCs = ""
    audioCount = hash["AudioList"].size
    
    hash["AudioList"].each do |audioTrack|
      audioCount = audioCount - 1

      #Bitrates
      audioBitrates << audioTrack["AudioBitrate"]
      
      #Encoders
      case audioTrack["AudioEncoder"]
        when /AC3 Pass/
          audioEncoders << "copy:ac3"
        when /AC3/
          audioEncoders << "ac3"
        when /DTS Pass/
          audioEncoders << "copy:dts"
        when /DTS-HD Pass/
          audioEncoders << "copy:dtshd"
        when /AAC Pass/
          audioEncoders << "copy:aac"
        when "AAC (ffmpeg)", "AAC (avcodec)", "AAC (faac)"
          audioEncoders << "av_aac"
        when "AAC (FDK)"
          audioEncoders << "fdk_aac"
        when "HE-AAC (FDK)"
          audioEncoders << "fdk_haac"
        when "AAC (CoreAudio)"
          audioEncoders << "ca_aac"
        when "HE-AAC (CoreAudio)"
          audioEncoders << "ca_haac"
        when /Vorbis/
          audioEncoders << "vorbis"
        when /MP3 Pass/
          audioEncoders << "copy:mp3"
        when /MP3/
          audioEncoders << "mp3"
        when "FLAC (ffmpeg)", "FLAC 16-bit"
          audioEncoders << "flac16"
        when "FLAC (24-bit)", "FLAC 24-bit"
          audioEncoders << "flac24"
        when /Auto Pass/
          audioEncoders << "copy"
      end
      
      #Mixdowns
      case audioTrack["AudioMixdown"]
        when "Mono (Left Only)"
          audioMixdowns << "left_only"
        when "Mono (Right Only)"
          audioMixdowns << "right_only"
        when /Mono/
          audioMixdowns << "mono"
        when /Stereo/
          audioMixdowns << "stereo"
        when /Dolby Surround/
          audioMixdowns << "dpl1"
        when /Dolby Pro Logic II/
          audioMixdowns << "dpl2"
        when /5.1/, /discrete/
          audioMixdowns << "5point1"
        when /6.1/
          audioMixdowns << "6point1"
        when "7.1 (5F/2R/LFE)"
          audioMixdowns << "5_2_lfe"
        when /7.1/
          audioMixdowns << "7point1"
        when /None/
          audioMixdowns << "none"
      end
      
      #Samplerates
      audioSamplerates << audioTrack["AudioSamplerate"]
      
      #Tracks
      audioTracks << audioTrack["AudioTrack"].to_s
      
      #DRC
      audioTrackDRCs << audioTrack["AudioTrackDRCSlider"].to_s
      
      if audioCount > 0
        audioBitrates << ","
        audioEncoders << ","
        audioMixdowns << ","
        audioSamplerates << ","
        audioTracks << ","
        audioTrackDRCs << ","
      end
      
    end
    commandString << " -a " << audioTracks
    commandString << " -E " << audioEncoders
    commandString << " -B " << audioBitrates
    commandString << " -6 " << audioMixdowns
    commandString << " -R " << audioSamplerates
    commandString << " -D " << audioTrackDRCs
    
    #Auto Passthru Mask
    audioCopyMask = ""
    
    if hash["AudioAllowAACPass"].to_i == 1
      audioCopyMask << "aac"
    end
    if hash["AudioAllowAC3Pass"].to_i == 1
      if audioCopyMask.size > 0
        audioCopyMask << ","
      end
      audioCopyMask << "ac3"
    end
    if hash["AudioAllowDTSHDPass"].to_i == 1
      if audioCopyMask.size > 0
        audioCopyMask << ","
      end
      audioCopyMask << "dtshd"
    end
    if hash["AudioAllowDTSPass"].to_i == 1
      if audioCopyMask.size > 0
        audioCopyMask << ","
      end
      audioCopyMask << "dts"
    end
    if hash["AudioAllowMP3Pass"].to_i == 1
      if audioCopyMask.size > 0
        audioCopyMask << ","
      end
      audioCopyMask << "mp3"
    end
    
    if audioCopyMask.size > 0
      commandString << " --audio-copy-mask " << audioCopyMask
    end
    
    #Auto Passthru Fallback
    audioEncoderFallback = ""
    
    case hash["AudioEncoderFallback"]
      when /AC3/
        audioEncoderFallback << "ac3"
      when "AAC (ffmpeg)", "AAC (avcodec)", "AAC (faac)"
        audioEncoderFallback << "av_aac"
      when "AAC (FDK)"
        audioEncoderFallback << "fdk_aac"
      when "HE-AAC (FDK)"
        audioEncoderFallback << "fdk_haac"
      when "AAC (CoreAudio)"
        audioEncoderFallback << "ca_aac"
      when "HE-AAC (CoreAudio)"
        audioEncoderFallback << "ca_haac"
      when /Vorbis/
        audioEncoderFallback << "vorbis"
      when /MP3/
        audioEncoderFallback << "mp3"
      when "FLAC (ffmpeg)", "FLAC 16-bit"
        audioEncoderFallback << "flac16"
      when "FLAC (24-bit)", "FLAC 24-bit"
        audioEncoderFallback << "flac24"
    end
    
    if audioEncoderFallback.size > 0
      commandString << " --audio-fallback " << audioEncoderFallback
    end
    
    #Container
    commandString << " -f "
    case hash["FileFormat"]
    when "MPEG-4 (mp4v2)"
      commandString << "mp4v2"
    when /MP4/
      commandString << "mp4"
    when "Matroska (libmkv)"
      commandString << "libmkv"
    when /MKV/
      commandString << "mkv"
    end
    
    #iPod MP4 atom
    if hash["Mp4iPodCompatible"].to_i == 1
      commandString << " -I"
    end
    
    #MP4 Optimize for HTTP Streaming
    if hash["Mp4HttpOptimize"].to_i == 1
      commandString << " -O"
    end
    
    #Cropping
    if hash["PictureAutoCrop"] == 0
      commandString << " --crop "
      commandString << hash["PictureTopCrop"].to_s
      commandString << ":"
      commandString << hash["PictureBottomCrop"].to_s
      commandString << ":"
      commandString << hash["PictureLeftCrop"].to_s
      commandString << ":"
      commandString << hash["PictureRightCrop"].to_s
    end
    
    #Dimensions
    if hash["PictureWidth"] != 0
      commandString << " -X "
      commandString << hash["PictureWidth"].to_s
    end
    if hash["PictureHeight"] != 0
      commandString << " -Y "
      commandString << hash["PictureHeight"].to_s
    end
    
    #Subtitles
    if hash["Subtitles"] && hash["Subtitles"] != "None"
      if hash["Subtitles"] == "Autoselect"
        commandString << " --subtitle-scan"
      else
        commandString << " -s "
        commandString << hash["Subtitles"]
      end
    end
    
    #Video Filters
    if hash["UsesPictureFilters"] == 1
      
      case hash["PictureDeinterlace"]
      when 1
        commandString << " --deinterlace=" << hash["PictureDeinterlaceCustom"].to_s
      when 2
        commandString << " --deinterlace=fast"
      when 3
        commandString << " --deinterlace=slow"
      when 4
        commandString << " --deinterlace=slower"
      when 5
        commandString << " --deinterlace=bob"
      end
      
      case hash["PictureDenoise"]
      when 1
        commandString << " --denoise=" << hash["PictureDenoiseCustom"].to_s
      when 2
        commandString << " --denoise=weak"
      when 3
        commandString << " --denoise=medium"
      when 4
        commandString << " --denoise=strong"
      end
      
      case hash["PictureDecomb"]
      when 1
        commandString << " --decomb=" << hash["PictureDecombCustom"].to_s
      when 2
        commandString << " --decomb"
      when 3
        commandString << " --decomb=fast"
      when 4
        commandString << " --decomb=bob"
      end

      case hash["PictureDetelecine"]
        when 1
          commandString << " --detelecine=" << hash["PictureDetelecineCustom"].to_s
        when 2
          commandString << " --detelecine"
      end

      if hash["PictureDeblock"] != 0
        commandString << " --deblock=" << hash["PictureDeblock"].to_s
      end

    end

    #Anamorphic
    if hash["PicturePAR"] == 1
      commandString << " --strict-anamorphic"
    elsif hash["PicturePAR"] == 2
      commandString << " --loose-anamorphic"
    elsif hash["PicturePAR"] == 3
      commandString << " --custom-anamorphic"
    end
    
    #Modulus
    if hash["PictureModulus"]
      commandString << " --modulus " << hash["PictureModulus"].to_s
    end

    #Booleans
    if hash["ChapterMarkers"] == 1 then commandString << " -m" end
    if hash["VideoGrayScale"] == 1 then commandString << " -g" end
    if hash["VideoTwoPass"] == 1 then commandString << " -2" end
    if hash["VideoTurboTwoPass"] == 1 then commandString << " -T" end

    #Advanced Options
    if hash["x264UseAdvancedOptions"] != 1
      if hash["x264Preset"] != ""
        commandString << " --x264-preset "
        commandString << hash["x264Preset"]
      end
      if hash["x264Tune"] != "" && hash["x264Tune"] != "none"
        commandString << " --x264-tune "
        commandString << hash["x264Tune"]
      end
      if hash["h264Profile"] != "" && hash["h264Profile"] != "auto"
        commandString << " --h264-profile "
        commandString << hash["h264Profile"]
      end
      if hash["h264Level"] != "" && hash["h264Level"] != "auto"
        commandString << " --h264-level "
        commandString << hash["h264Level"]
      end
      if hash["x264OptionExtra"] != ""
        commandString << " -x "
        commandString << hash["x264OptionExtra"]
      end
    elsif hash["x264Option"] != ""
      commandString << " -x "
      commandString << hash["x264Option"]
    end
    
    # That's it, print to screen now
    puts commandString
  end

  def generateAPIcalls(hash) # Makes a C version of the preset ready for coding into the CLI
    
    commandString = "if (!strcasecmp(preset_name, \"" << hash["PresetName"] << "\"))\n{\n    "
    
    #Container
    commandString << "if( !mux )\n    "
    commandString << "{\n    "

    case hash["FileFormat"]
    when "MPEG-4 (mp4v2)"
      commandString << "    mux = " << "HB_MUX_MP4V2;\n    "
    when /MP4/
      commandString << "    mux = " << "HB_MUX_MP4;\n    "
    when "Matroska (libmkv)"
      commandString << "    mux = " << "HB_MUX_LIBMKV;\n    "
    when /MKV/
      commandString << "    mux = " << "HB_MUX_MKV;\n    "
    end
    commandString << "}\n    "
    
    #iPod MP4 atom
    if hash["Mp4iPodCompatible"].to_i == 1
      commandString << "job->ipod_atom = 1;\n    "
    end
    
    #MP4 Optimize for HTTP Streaming
    if hash["Mp4HttpOptimize"].to_i == 1
      commandString << "job->mp4_optimize = 1;\n    "
    end
    
    #Video encoder
    commandString << "vcodec = "
    case hash["VideoEncoder"]
    when /x264/
      commandString << "HB_VCODEC_X264;\n    "
    when /Theora/
      commandString << "HB_VCODEC_THEORA;\n    "
    when /MPEG-4/
      commandString << "HB_VCODEC_FFMPEG_MPEG4;\n    "
    when /MPEG-2/
      commandString << "HB_VCODEC_FFMPEG_MPEG2;\n    "
    end

    #VideoRateControl
    case hash["VideoQualityType"]
    when 0
      commandString << "size = " << hash["VideoTargetSize"] << ";\n    "
    when 1
      commandString << "job->vbitrate = " << hash["VideoAvgBitrate"] << ";\n    "
    when 2
      commandString << "job->vquality = " << hash["VideoQualitySlider"].to_s << ";\n    "
    end

    #FPS
    if hash["VideoFramerate"] != "Same as source"
      if hash["VideoFramerate"] == "23.976 (NTSC Film)"
        commandString << "filter_vrate_base = " << "1126125;\n    "
      elsif hash["VideoFramerate"] == "29.97 (NTSC Video)"
        commandString << "filter_vrate_base = " << "900900;\n    "
      elsif hash["VideoFramerate"] == "25 (PAL Film/Video)"
        commandString << "filter_vrate_base = " << "1080000;\n    "
      else
        commandString << "filter_vrate_base = " << (27000000 / hash["VideoFramerate"].to_i).to_s << ";\n    "
      end
      # not same as source: pfr, else default (cfr)
      if hash["VideoFramerateMode"] == "pfr"
        commandString << "filter_cfr = 2;\n    "
      else
        commandString << "filter_cfr = 1;\n    "
      end
    # same as source: cfr, else default (vfr)
    elsif hash["VideoFramerateMode"] == "cfr"
      commandString << "filter_cfr = 1;\n    "
    end
    
    #Audio tracks
    audioBitrates = ""
    audioEncoders = ""
    audioMixdowns = ""
    audioSamplerates = ""
    audioTracks = ""
    audioTrackDRCs = ""
    audioCount = hash["AudioList"].size

    hash["AudioList"].each do |audioTrack|
      audioCount = audioCount - 1

      #Bitrates
      audioBitrates << audioTrack["AudioBitrate"]

      #Encoders
      case audioTrack["AudioEncoder"]
        when /AC3 Pass/
          audioEncoders << "copy:ac3"
        when /AC3/
          audioEncoders << "ac3"
        when /DTS Pass/
          audioEncoders << "copy:dts"
        when /DTS-HD Pass/
          audioEncoders << "copy:dtshd"
        when /AAC Pass/
          audioEncoders << "copy:aac"
        when "AAC (ffmpeg)", "AAC (avcodec)", "AAC (faac)"
          audioEncoders << "av_aac"
        when "AAC (FDK)"
          audioEncoders << "fdk_aac"
        when "HE-AAC (FDK)"
          audioEncoders << "fdk_haac"
        when "AAC (CoreAudio)"
          audioEncoders << "ca_aac"
        when "HE-AAC (CoreAudio)"
          audioEncoders << "ca_haac"
        when /Vorbis/
          audioEncoders << "vorbis"
        when /MP3 Pass/
          audioEncoders << "copy:mp3"
        when /MP3/
          audioEncoders << "mp3"
        when "FLAC (ffmpeg)", "FLAC 16-bit"
          audioEncoders << "flac16"
        when "FLAC (24-bit)", "FLAC 24-bit"
          audioEncoders << "flac24"
        when /Auto Pass/
          audioEncoders << "copy"
      end

      #Mixdowns
      case audioTrack["AudioMixdown"]
        when "Mono (Left Only)"
          audioMixdowns << "left_only"
        when "Mono (Right Only)"
          audioMixdowns << "right_only"
        when /Mono/
          audioMixdowns << "mono"
        when /Stereo/
          audioMixdowns << "stereo"
        when /Dolby Surround/
          audioMixdowns << "dpl1"
        when /Dolby Pro Logic II/
          audioMixdowns << "dpl2"
        when /5.1/, /discrete/
          audioMixdowns << "5point1"
        when /6.1/
          audioMixdowns << "6point1"
        when "7.1 (5F/2R/LFE)"
          audioMixdowns << "5_2_lfe"
        when /7.1/
          audioMixdowns << "7point1"
        when /None/
          audioMixdowns << "none"
      end

      #Samplerates
      audioSamplerates << audioTrack["AudioSamplerate"]

      #Tracks
      audioTracks << audioTrack["AudioTrack"].to_s

      #DRC
      audioTrackDRCs << audioTrack["AudioTrackDRCSlider"].to_s

      if audioCount > 0
        audioBitrates << ","
        audioEncoders << ","
        audioMixdowns << ","
        audioSamplerates << ","
        audioTracks << ","
        audioTrackDRCs << ","
      end

    end
    commandString << "if( !atracks )\n    "
    commandString << "{\n    "
    commandString << "    atracks = strdup(\"" << audioTracks
    commandString << "\");\n    "
    commandString << "}\n    "

    commandString << "if( !acodecs )\n    "
    commandString << "{\n    "
    commandString << "    acodecs = strdup(\"" << audioEncoders
    commandString << "\");\n    "
    commandString << "}\n    "

    commandString << "if( !abitrates )\n    "
    commandString << "{\n    "
    commandString << "    abitrates = str_split(\"" << audioBitrates
    commandString << "\", ',');\n    "
    commandString << "}\n    "

    commandString << "if( !mixdowns )\n    "
    commandString << "{\n    "
    commandString << "    mixdowns = strdup(\"" << audioMixdowns
    commandString << "\");\n    "
    commandString << "}\n    "

    commandString << "if( !arates )\n    "
    commandString << "{\n    "
    commandString << "    arates = strdup(\"" << audioSamplerates
    commandString << "\");\n    "
    commandString << "}\n    "

    commandString << "if( !dynamic_range_compression )\n    "
    commandString << "{\n    "
    commandString << "    dynamic_range_compression = strdup(\"" << audioTrackDRCs
    commandString << "\");\n    "
    commandString << "}\n    "
    
    #Auto Passthru Mask
    if hash["AudioAllowAACPass"]
      commandString << "if( allowed_audio_copy == -1 )\n    "
      commandString << "{\n    "
      commandString << "    allowed_audio_copy = 0;\n    "
      if hash["AudioAllowAACPass"].to_i == 1
        commandString << "    allowed_audio_copy |= HB_ACODEC_AAC_PASS;\n    "
      end
      if hash["AudioAllowAC3Pass"].to_i == 1
        commandString << "    allowed_audio_copy |= HB_ACODEC_AC3_PASS;\n    "
      end
      if hash["AudioAllowDTSHDPass"].to_i == 1
        commandString << "    allowed_audio_copy |= HB_ACODEC_DCA_HD_PASS;\n    "
      end
      if hash["AudioAllowDTSPass"].to_i == 1
        commandString << "    allowed_audio_copy |= HB_ACODEC_DCA_PASS;\n    "
      end
      if hash["AudioAllowMP3Pass"].to_i == 1
        commandString << "    allowed_audio_copy |= HB_ACODEC_MP3_PASS;\n    "
      end
      commandString << "    allowed_audio_copy &= HB_ACODEC_PASS_MASK;\n    "
      commandString << "}\n    "
    end
    
    #Auto Passthru Fallback
    audioEncoderFallback = ""
    
    case hash["AudioEncoderFallback"]
      when /AC3/
        audioEncoderFallback << "ac3"
      when "AAC (ffmpeg)", "AAC (avcodec)", "AAC (faac)"
        audioEncoderFallback << "av_aac"
      when "AAC (FDK)"
        audioEncoderFallback << "fdk_aac"
      when "HE-AAC (FDK)"
        audioEncoderFallback << "fdk_haac"
      when "AAC (CoreAudio)"
        audioEncoderFallback << "ca_aac"
      when "HE-AAC (CoreAudio)"
        audioEncoderFallback << "ca_haac"
      when /Vorbis/
        audioEncoderFallback << "vorbis"
      when /MP3/
        audioEncoderFallback << "mp3"
      when "FLAC (ffmpeg)", "FLAC 16-bit"
        audioEncoderFallback << "flac16"
      when "FLAC (24-bit)", "FLAC 24-bit"
        audioEncoderFallback << "flac24"
    end
    
    if audioEncoderFallback.size > 0
      commandString << "if( acodec_fallback == NULL )\n    "
      commandString << "{\n    "
      commandString << "    acodec_fallback = \"" << audioEncoderFallback << "\";\n    "
      commandString << "}\n    "
    end
    
    #Cropping
    if hash["PictureAutoCrop"] == 0
      commandString << "job->crop[0] = " << hash["PictureTopCrop"].to_s << ";\n    "
      commandString << "job->crop[1] = " << hash["PictureBottomCrop"].to_s << ";\n    "
      commandString << "job->crop[2] = " << hash["PictureLeftCrop"].to_s << ";\n    "
      commandString << "job->crop[4] = " << hash["PictureRightCrop"].to_s << ";\n    "
    end
    
    #Dimensions
    if hash["PictureWidth"] != 0
      commandString << "maxWidth = "
      commandString << hash["PictureWidth"].to_s << ";\n    "
    end
    if hash["PictureHeight"] != 0
      commandString << "maxHeight = "
      commandString << hash["PictureHeight"].to_s << ";\n    "
    end
    
    #Subtitles
    if hash["Subtitles"] != "None"
      if hash["Subtitles"] == "Autoselect"
        commandString << "subtitle_scan = 1;\n    "
      else
        commandString << "job->subtitle = "
        commandString << ( hash["Subtitles"].to_i - 1).to_s << ";\n    "
      end
    end
    
    #Advanced Options
    if hash["x264UseAdvancedOptions"] != 1
      if hash["x264Preset"] != ""
        commandString << "if (x264_preset == NULL)\n    "
        commandString << "{\n    "
        commandString << "    x264_preset = strdup(\""
        commandString << hash["x264Preset"] << "\");\n    "
        commandString << "}\n    "
      end
      if hash["x264Tune"] != "" && hash["x264Tune"] != "none"
        commandString << "if (x264_tune == NULL)\n    "
        commandString << "{\n    "
        commandString << "    x264_tune = strdup(\""
        commandString << hash["x264Tune"]
        commandString << "\");\n    "
        commandString << "}\n    "
      end
      if hash["h264Profile"] != "" && hash["h264Profile"] != "auto"
        commandString << "if (h264_profile == NULL)\n    "
        commandString << "{\n    "
        commandString << "    h264_profile = strdup(\""
        commandString << hash["h264Profile"] << "\");\n    "
        commandString << "}\n    "
      end
      if hash["h264Level"] != "" && hash["h264Level"] != "auto"
        commandString << "if (h264_level == NULL)\n    "
        commandString << "{\n    "
        commandString << "    h264_level = strdup(\""
        commandString << hash["h264Level"] << "\");\n    "
        commandString << "}\n    "
      end
      if hash["x264OptionExtra"] != ""
        commandString << "if (advanced_opts == NULL)\n    "
        commandString << "{\n    "
        commandString << "    advanced_opts = strdup(\""
        commandString << hash["x264OptionExtra"] << "\");\n    "
        commandString << "}\n    "
      end
    elsif hash["x264Option"] != ""
      commandString << "if (advanced_opts == NULL)\n    "
      commandString << "{\n    "
      commandString << "    advanced_opts = strdup(\""
      commandString << hash["x264Option"] << "\");\n    "
      commandString << "}\n    "
    end
    
    #Video Filters
    if hash["UsesPictureFilters"] == 1
      
      if hash["PictureDeinterlace"].to_i != 0
        commandString << "deinterlace = 1;\n    "
      end

      case hash["PictureDeinterlace"]
      when 1
        commandString << "deinterlace_opt = \"" << hash["PictureDeinterlaceCustom"].to_s << "\";\n    "
      when 2
        commandString << "deinterlace_opt = \"0\";\n    "
      when 3
        commandString << "deinterlace_opt = \"1\";\n    "
      when 4
        commandString << "deinterlace_opt = \"3\";\n    "
      when 5
        commandString << "deinterlace_opt = \"15\";\n    "
      end
      
      if hash["PictureDenoise"].to_i != 0
        commandString << "denoise = 1;\n    "
      end

      case hash["PictureDenoise"]
      when 1
        commandString << "denoise_opt = \"" << hash["PictureDenoiseCustom"].to_s << "\";\n    "
      when 2
        commandString << "denoise_opt = \"2:1:1:2:3:3\";\n    "
      when 3
        commandString << "denoise_opt = \"3:2:2:2:3:3\";\n    "
      when 4
        commandString << "denoise_opt = \"7:7:7:5:5:5\";\n    "
      end
      
      if hash["PictureDecomb"].to_i != 0
        commandString << "decomb = 1;\n    "
      end

      case hash["PictureDecomb"]
      when 1
        commandString << "decomb_opt = \"" << hash["PictureDecombCustom"].to_s << "\";\n    "
      when 3
        commandString << "decomb_opt = \"7:2:6:9:1:80\";\n    "
      when 4
        commandString << "decomb_opt = \"455\";\n    "
      end

      if hash["PictureDetelecine"].to_i != 0
        commandString << "detelecine = 1;\n    "
      end

      case hash["PictureDetelecine"]
        when 1
          commandString << "detelecine_opt = \"" << hash["PictureDetelecineCustom"].to_s << "\";\n    "
      end

      if hash["PictureDeblock"] != 0
        commandString << "deblock = 1;\n    "
        commandString << "deblock_opt = \"" << hash["PictureDeblock"].to_s << "\";\n    "
      end
      
    end
    
    #Anamorphic
    if hash["PicturePAR"] != 0
      commandString << "if( !anamorphic_mode )\n    "
      commandString << "{\n    "
      if hash["PicturePAR"] == 1
        commandString << "    anamorphic_mode = 1;\n    "
      elsif hash["PicturePAR"] == 2
        commandString << "    anamorphic_mode = 2;\n    "
      elsif hash["PicturePAR"] == 3
        commandString << "    anamorphic_mode = 3;\n    "
      end
      commandString << "}\n    "
    end
    
    #Modulus
    if hash["PictureModulus"]
      commandString << "modulus = " << hash["PictureModulus"].to_s << ";\n    "
    end

    #Booleans
    if hash["ChapterMarkers"] == 1
      commandString << "job->chapter_markers = 1;\n    "
    end

    if hash["VideoGrayScale"] == 1
      commandString << "job->grayscale = 1;\n    "
    end

    if hash["VideoTwoPass"] == 1
      commandString << "twoPass = 1;\n    "
    end

    if hash["VideoTurboTwoPass"] == 1
      commandString << "turbo_opts_enabled = 1;\n    "
    end

    #Finish
    commandString = commandString.rstrip
    commandString << "\n}"

    # That's it, print to screen now
    puts commandString
  end
  
  def generateAPIFolderList( hash, depth )
    commandString = ""
    
    commandString << "    printf(\"\\n"
    depth.times do
      commandString << "   "
    end
    (depth+1).times do
      commandString << "<"
    end
    commandString << " " << hash["PresetName"]
    commandString << "\\n\");\n"    
    puts commandString
  end
  
  def generateAPIFolderCloserList( depth )
    commandString = ""
    
    commandString << "    printf(\"\\n"
    depth.times do
      commandString << "   "
    end
    (depth+1).times do
      commandString << ">"
    end
    commandString << "\\n\");\n"
    puts commandString
  end
  
  def generateAPIList(hash, depth) # Makes a list of the CLI options a built-in CLI preset uses, for wrappers to parse
    commandString = ""
    commandString << "    printf(\"\\n"
    depth.times do
      commandString << "   "
    end
           
    commandString << "+ " << hash["PresetName"] << ": "
        
    #Video encoder
    commandString << " -e "
    case hash["VideoEncoder"]
    when /x264/
      commandString << "x264 "
    when /Theora/
      commandString << "theora "
    when /MPEG-4/
      commandString << "ffmpeg4 "
    when /MPEG-2/
      commandString << "ffmpeg2 "
    end

    #VideoRateControl
    case hash["VideoQualityType"]
    when 0
      commandString << " -S " << hash["VideoTargetSize"]
    when 1
      commandString << " -b " << hash["VideoAvgBitrate"]
    when 2
      commandString << " -q " << hash["VideoQualitySlider"].to_s
    end

    #FPS
    if hash["VideoFramerate"] != "Same as source"
      if hash["VideoFramerate"] == "23.976 (NTSC Film)"
        commandString << " -r " << "23.976"
      elsif hash["VideoFramerate"] == "29.97 (NTSC Video)"
        commandString << " -r " << "29.97"
      elsif hash["VideoFramerate"] == "25 (PAL Film/Video)"
        commandString << " -r " << "25"
      else
        commandString << " -r " << hash["VideoFramerate"]
      end
      # not same as source: pfr, else default (cfr)
      if hash["VideoFramerateMode"] == "pfr"
        commandString << " --pfr "
      end
    # same as source: cfr, else default (vfr)
    elsif hash["VideoFramerateMode"] == "cfr"
      commandString << " --cfr "
    end
    
    #Audio tracks
    audioBitrates = ""
    audioEncoders = ""
    audioMixdowns = ""
    audioSamplerates = ""
    audioTracks = ""
    audioTrackDRCs = ""
    audioCount = hash["AudioList"].size
    
    hash["AudioList"].each do |audioTrack|
      audioCount = audioCount - 1

      #Bitrates
      audioBitrates << audioTrack["AudioBitrate"]
      
      #Encoders
      case audioTrack["AudioEncoder"]
        when /AC3 Pass/
          audioEncoders << "copy:ac3"
        when /AC3/
          audioEncoders << "ac3"
        when /DTS Pass/
          audioEncoders << "copy:dts"
        when /DTS-HD Pass/
          audioEncoders << "copy:dtshd"
        when /AAC Pass/
          audioEncoders << "copy:aac"
        when "AAC (ffmpeg)", "AAC (avcodec)", "AAC (faac)"
          audioEncoders << "av_aac"
        when "AAC (FDK)"
          audioEncoders << "fdk_aac"
        when "HE-AAC (FDK)"
          audioEncoders << "fdk_haac"
        when "AAC (CoreAudio)"
          audioEncoders << "ca_aac"
        when "HE-AAC (CoreAudio)"
          audioEncoders << "ca_haac"
        when /Vorbis/
          audioEncoders << "vorbis"
        when /MP3 Pass/
          audioEncoders << "copy:mp3"
        when /MP3/
          audioEncoders << "mp3"
        when "FLAC (ffmpeg)", "FLAC 16-bit"
          audioEncoders << "flac16"
        when "FLAC (24-bit)", "FLAC 24-bit"
          audioEncoders << "flac24"
        when /Auto Pass/
          audioEncoders << "copy"
      end
      
      #Mixdowns
      case audioTrack["AudioMixdown"]
        when "Mono (Left Only)"
          audioMixdowns << "left_only"
        when "Mono (Right Only)"
          audioMixdowns << "right_only"
        when /Mono/
          audioMixdowns << "mono"
        when /Stereo/
          audioMixdowns << "stereo"
        when /Dolby Surround/
          audioMixdowns << "dpl1"
        when /Dolby Pro Logic II/
          audioMixdowns << "dpl2"
        when /5.1/, /discrete/
          audioMixdowns << "5point1"
        when /6.1/
          audioMixdowns << "6point1"
        when "7.1 (5F/2R/LFE)"
          audioMixdowns << "5_2_lfe"
        when /7.1/
          audioMixdowns << "7point1"
        when /None/
          audioMixdowns << "none"
      end
      
      #Samplerates
      audioSamplerates << audioTrack["AudioSamplerate"]
      
      #Tracks
      audioTracks << audioTrack["AudioTrack"].to_s
      
      #DRC
      audioTrackDRCs << audioTrack["AudioTrackDRCSlider"].to_s
      
      if audioCount > 0
        audioBitrates << ","
        audioEncoders << ","
        audioMixdowns << ","
        audioSamplerates << ","
        audioTracks << ","
        audioTrackDRCs << ","
      end
      
    end
    commandString << " -a " << audioTracks
    commandString << " -E " << audioEncoders
    commandString << " -B " << audioBitrates
    commandString << " -6 " << audioMixdowns
    commandString << " -R " << audioSamplerates
    commandString << " -D " << audioTrackDRCs
    
    #Auto Passthru Mask
    audioCopyMask = ""
    
    if hash["AudioAllowAACPass"].to_i == 1
      audioCopyMask << "aac"
    end
    if hash["AudioAllowAC3Pass"].to_i == 1
      if audioCopyMask.size > 0
        audioCopyMask << ","
      end
      audioCopyMask << "ac3"
    end
    if hash["AudioAllowDTSHDPass"].to_i == 1
      if audioCopyMask.size > 0
        audioCopyMask << ","
      end
      audioCopyMask << "dtshd"
    end
    if hash["AudioAllowDTSPass"].to_i == 1
      if audioCopyMask.size > 0
        audioCopyMask << ","
      end
      audioCopyMask << "dts"
    end
    if hash["AudioAllowMP3Pass"].to_i == 1
      if audioCopyMask.size > 0
        audioCopyMask << ","
      end
      audioCopyMask << "mp3"
    end
    
    if audioCopyMask.size > 0
      commandString << " --audio-copy-mask " << audioCopyMask
    end
    
    #Auto Passthru Fallback
    audioEncoderFallback = ""
    
    case hash["AudioEncoderFallback"]
      when /AC3/
        audioEncoderFallback << "ac3"
      when "AAC (ffmpeg)", "AAC (avcodec)", "AAC (faac)"
        audioEncoderFallback << "av_aac"
      when "AAC (FDK)"
        audioEncoderFallback << "fdk_aac"
      when "HE-AAC (FDK)"
        audioEncoderFallback << "fdk_haac"
      when "AAC (CoreAudio)"
        audioEncoderFallback << "ca_aac"
      when "HE-AAC (CoreAudio)"
        audioEncoderFallback << "ca_haac"
      when /Vorbis/
        audioEncoderFallback << "vorbis"
      when /MP3/
        audioEncoderFallback << "mp3"
      when "FLAC (ffmpeg)", "FLAC 16-bit"
        audioEncoderFallback << "flac16"
      when "FLAC (24-bit)", "FLAC 24-bit"
        audioEncoderFallback << "flac24"
    end
    
    if audioEncoderFallback.size > 0
      commandString << " --audio-fallback " << audioEncoderFallback
    end
    
    #Container
    commandString << " -f "
    case hash["FileFormat"]
    when "MPEG-4 (mp4v2)"
      commandString << "mp4v2"
    when /MP4/
      commandString << "mp4"
    when "Matroska (libmkv)"
      commandString << "libmkv"
    when /MKV/
      commandString << "mkv"
    end
    
    #iPod MP4 atom
    if hash["Mp4iPodCompatible"].to_i == 1
      commandString << " -I"
    end
    
    #MP4 Optimize for HTTP Streaming
    if hash["Mp4HttpOptimize"].to_i == 1
      commandString << " -O"
    end
    
    #Cropping
    if hash["PictureAutoCrop"] == 0
      commandString << " --crop "
      commandString << hash["PictureTopCrop"].to_s
      commandString << ":"
      commandString << hash["PictureBottomCrop"].to_s
      commandString << ":"
      commandString << hash["PictureLeftCrop"].to_s
      commandString << ":"
      commandString << hash["PictureRightCrop"].to_s
    end
    
    #Dimensions
    if hash["PictureWidth"] != 0
      commandString << " -X "
      commandString << hash["PictureWidth"].to_s
    end
    if hash["PictureHeight"] != 0
      commandString << " -Y "
      commandString << hash["PictureHeight"].to_s
    end
    
    #Subtitles
    if hash["Subtitles"] && hash["Subtitles"] != "None"
      if hash["Subtitles"] == "Autoselect"
        commandString << " --subtitle-scan"
      else
        commandString << " -s "
        commandString << hash["Subtitles"]
      end
    end
    
    #Video Filters
    if hash["UsesPictureFilters"] == 1
      
      case hash["PictureDeinterlace"]
      when 1
        commandString << " --deinterlace=" << hash["PictureDeinterlaceCustom"].to_s
      when 2
        commandString << " --deinterlace=fast"
      when 3
        commandString << " --deinterlace=slow"
      when 4
        commandString << " --deinterlace=slower"
      when 5
        commandString << " --deinterlace=bob"
      end
      
      case hash["PictureDenoise"]
      when 1
        commandString << " --denoise=" << hash["PictureDenoiseCustom"].to_s
      when 2
        commandString << " --denoise=weak"
      when 3
        commandString << " --denoise=medium"
      when 4
        commandString << " --denoise=strong"
      end
      
      case hash["PictureDecomb"]
      when 1
        commandString << " --decomb=" << hash["PictureDecombCustom"].to_s
      when 2
        commandString << " --decomb"
      when 3
        commandString << " --decomb=fast"
      when 4
        commandString << " --decomb=bob"
      end

      case hash["PictureDetelecine"]
        when 1
          commandString << " --detelecine=" << hash["PictureDetelecineCustom"].to_s
        when 2
          commandString << " --detelecine"
      end

      if hash["PictureDeblock"] != 0
        commandString << " --deblock=" << hash["PictureDeblock"].to_s
      end

    end
    
    #Anamorphic
    if hash["PicturePAR"] == 1
      commandString << " --strict-anamorphic"
    elsif hash["PicturePAR"] == 2
      commandString << " --loose-anamorphic"
    elsif hash["PicturePAR"] == 3
      commandString << " --custom-anamorphic"
    end
    
    #Modulus
    if hash["PictureModulus"]
      commandString << " --modulus " << hash["PictureModulus"].to_s
    end

    #Booleans
    if hash["ChapterMarkers"] == 1 then commandString << " -m" end
    if hash["VideoGrayScale"] == 1 then commandString << " -g" end
    if hash["VideoTwoPass"] == 1 then commandString << " -2" end
    if hash["VideoTurboTwoPass"] == 1 then commandString << " -T" end
    
    #Advanced Options
    if hash["x264UseAdvancedOptions"] != 1
      if hash["x264Preset"] != ""
        commandString << " --x264-preset "
        commandString << hash["x264Preset"]
      end
      if hash["x264Tune"] != "" && hash["x264Tune"] != "none"
        commandString << " --x264-tune "
        commandString << hash["x264Tune"]
      end
      if hash["h264Profile"] != "" && hash["h264Profile"] != "auto"
        commandString << " --h264-profile "
        commandString << hash["h264Profile"]
      end
      if hash["h264Level"] != "" && hash["h264Level"] != "auto"
        commandString << " --h264-level "
        commandString << hash["h264Level"]
      end
      if hash["x264OptionExtra"] != ""
        commandString << " -x "
        commandString << hash["x264OptionExtra"]
      end
    elsif hash["x264Option"] != ""
      commandString << " -x "
      commandString << hash["x264Option"]
    end
    
    commandString << "\\n\");"
    
    # That's it, print to screen now
    puts commandString
  end
  
end

# CLI invocation only
if __FILE__ == $0

  # First grab the specified CLI options
  options = readOptions

  # Only run if one of the useful CLI flags have been passed
  if options.cliraw == true || options.cliparse == true || options.api == true || options.apilist == true
    # This line is the ignition -- generates hashes of
    # presets and then displays them to the screen
    # with the options the user selects on the CLI. 
    Display.new( Presets.new.hashMasterList, options )
  else
    # Direct the user to the help
    puts "\n\tUsage: manicure.rb [options]"
    puts "\tSee help with -h or --help"
  end

end
