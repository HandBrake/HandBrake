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
    commandString << " " << hash["PresetName"] << "\n\n"
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
    commandString << "\n\n"
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
    when /MP4/
      commandString << "mp4 "
    when /AVI/
      commandString << "avi "
    when /OGM/
      commandString << "ogm "
    when /MKV/
      commandString << "mkv "
    end
    
    #Video encoder
    if hash["VideoEncoder"] != "MPEG-4 (FFmpeg)"
      commandString << " -e "
      case hash["VideoEncoder"]
      when /x264/
        commandString << "x264"
      when /XviD/
        commandString << "xvid"
      end
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
        when /AC3 /
          audioEncoders << "ac3"
        when /AAC/
          audioEncoders << "faac"
        when /Vorbis/
          audioEncoders << "vorbis"
        when /MP3/
          audioEncoders << "lame"
      end
      
      #Mixdowns
      case audioTrack["AudioMixdown"]
      when /Mono/
        audioMixdowns << "mono"
      when /Stereo/
        audioMixdowns << "stereo"
      when /Dolby Surround/
        audioMixdowns << "dpl1"
      when /Dolby Pro Logic II/
        audioMixdowns << "dpl2"
      when /discrete/
        audioMixdowns << "6ch"
      when /Passthru/
        audioMixdowns << "auto"
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
        
    #Container
    commandString << " -f "
    case hash["FileFormat"]
    when /MP4/
      commandString << "mp4"
    when /AVI/
      commandString << "avi"
    when /OGM/
      commandString << "ogm"
    when /MKV/
      commandString << "mkv"
    end
    
    #iPod MP4 atom
    if hash["Mp4iPodCompatible"].to_i == 1
      commandString << " -I"
    end
    
    # 64-bit files
    if hash["Mp4LargeFile"] == 1
      commandString << " -4"
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
    if hash["Subtitles"] != "None"
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
        commandString << " --deinterlace=\"fast\""
      when 2
        commandString << " --deinterlace=\slow\""
      when 3
        commandString << " --deinterlace=\"slower\""
      when 4
        commandString << " --deinterlace=\"slowest\""
      end
      
      case hash["PictureDenoise"]
      when 1
        commandString << " --denoise=\"weak\""
      when 2
        commandString << " --denoise=\"medium\""
      when 3
        commandString << " --denoise=\"strong\""
      end
      
      if hash["PictureDetelecine"] == 1 then commandString << " --detelecine" end
      if hash["PictureDeblock"] == 1 then commandString << " --deblock" end
      if hash["VFR"].to_i == 1 then commandString << " --vfr" end
      if hash["PictureDecomb"] == 1 then commandString << " --decomb" end
      
    end
    
    #Anamorphic
    if hash["PicturePAR"] == 1
      commandString << " --strict-anamorphic"
    elsif hash["PicturePAR"] == 2
      commandString << " --loose-anamorphic"
    end

    #Booleans
    if hash["ChapterMarkers"] == 1 then commandString << " -m" end
    if hash["VideoGrayScale"] == 1 then commandString << " -g" end
    if hash["VideoTwoPass"] == 1 then commandString << " -2" end
    if hash["VideoTurboTwoPass"] == 1 then commandString << " -T" end

    #x264 Options
    if hash["x264Option"] != ""
      commandString << " -x "
      commandString << hash["x264Option"]
    end
    
    # That's it, print to screen now
    puts commandString
    
    #puts "*" * @columnWidth

    puts  "\n"
  end
  
  def generateCLIFolderParse( hash, depth ) # Shows the folder for wrappers to parse
    commandString = ""
    depth.times do
      commandString << "   "
    end
    (depth+1).times do
      commandString << "<"
    end
    commandString << " " << hash["PresetName"] << "\n\n"
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
    commandString << "\n\n"
    puts commandString
  end
  
  def generateCLIParse(hash, depth) # Makes a CLI equivalent of all user presets, for wrappers to parse
    commandString = ""
    depth.times do
      commandString << "   "
    end
    commandString << '+ ' << hash["PresetName"] << ":"
        
    #Video encoder
    if hash["VideoEncoder"] != "MPEG-4 (FFmpeg)"
      commandString << " -e "
      case hash["VideoEncoder"]
      when /x264/
        commandString << "x264"
      when /XviD/
        commandString << "xvid"
      end
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
        when /AC3 /
          audioEncoders << "ac3"
        when /AAC/
          audioEncoders << "faac"
        when /Vorbis/
          audioEncoders << "vorbis"
        when /MP3/
          audioEncoders << "lame"
      end
      
      #Mixdowns
      case audioTrack["AudioMixdown"]
      when /Mono/
        audioMixdowns << "mono"
      when /Stereo/
        audioMixdowns << "stereo"
      when /Dolby Surround/
        audioMixdowns << "dpl1"
      when /Dolby Pro Logic II/
        audioMixdowns << "dpl2"
      when /discrete/
        audioMixdowns << "6ch"
      when /Passthru/
        audioMixdowns << "auto"
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
    
    #Container
    commandString << " -f "
    case hash["FileFormat"]
    when /MP4/
      commandString << "mp4"
    when /AVI/
      commandString << "avi"
    when /OGM/
      commandString << "ogm"
    when /MKV/
      commandString << "mkv"
    end
    
    #iPod MP4 atom
    if hash["Mp4iPodCompatible"].to_i == 1
      commandString << " -I"
    end
    
    # 64-bit files
    if hash["Mp4LargeFile"] == 1
      commandString << " -4"
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
    if hash["Subtitles"] != "None"
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
        commandString << " --deinterlace=\"fast\""
      when 2
        commandString << " --deinterlace=\slow\""
      when 3
        commandString << " --deinterlace=\"slower\""
      when 4
        commandString << " --deinterlace=\"slowest\""
      end
      
      case hash["PictureDenoise"]
      when 1
        commandString << " --denoise=\"weak\""
      when 2
        commandString << " --denoise=\"medium\""
      when 3
        commandString << " --denoise=\"strong\""
      end
      
      if hash["PictureDetelecine"] == 1 then commandString << " --detelecine" end
      if hash["PictureDeblock"] == 1 then commandString << " --deblock" end
      if hash["VFR"].to_i == 1 then commandString << " --vfr" end
      if hash["PictureDecomb"] == 1 then commandString << " --decomb" end
    end

    #Anamorphic
    if hash["PicturePAR"] == 1
      commandString << " --strict-anamorphic"
    elsif hash["PicturePAR"] == 2
      commandString << " --loose-anamorphic"
    end
    
    #Booleans
    if hash["ChapterMarkers"] == 1 then commandString << " -m" end
    if hash["VideoGrayScale"] == 1 then commandString << " -g" end
    if hash["VideoTwoPass"] == 1 then commandString << " -2" end
    if hash["VideoTurboTwoPass"] == 1 then commandString << " -T" end

    #x264 Options
    if hash["x264Option"] != ""
      commandString << " -x "
      commandString << hash["x264Option"]
    end
    
    # That's it, print to screen now
    puts commandString
    
    #puts "*" * @columnWidth

    puts  "\n"
  end

  def generateAPIcalls(hash) # Makes a C version of the preset ready for coding into the CLI
    
    commandString = "if (!strcmp(preset_name, \"" << hash["PresetName"] << "\"))\n{\n    "
    
    #Filename suffix
    commandString << "if( !mux )\n    "
    commandString << "{\n    "

    case hash["FileFormat"]
    when /MP4/
      commandString << "    mux = " << "HB_MUX_MP4;\n    "
    when /AVI/
      commandString << "    mux = " << "HB_MUX_AVI;\n    "
    when /OGM/
      commandString << "    mux = " << "HB_MUX_OGM;\n    "
    when /MKV/
      commandString << "    mux = " << "HB_MUX_MKV;\n    "
    end
    commandString << "}\n    "
    
    #iPod MP4 atom
    if hash["Mp4iPodCompatible"].to_i == 1
      commandString << "job->ipod_atom = 1;\n   "
    end
    
    # 64-bit files
    if hash["Mp4LargeFile"] == 1
      commandString << "job->largeFileSize = 1;\n    "
    end
    
    #Video encoder
    if hash["VideoEncoder"] != "MPEG-4 (FFmpeg)"
      commandString << "vcodec = "
      case hash["VideoEncoder"]
      when /x264/
        commandString << "HB_VCODEC_X264;\n    "
      when /XviD/
        commandString << "HB_VCODEC_XVID;\n    "        
      end
    end

    #VideoRateControl
    case hash["VideoQualityType"]
    when 0
      commandString << "size = " << hash["VideoTargetSize"] << ";\n    "
    when 1
      commandString << "job->vbitrate = " << hash["VideoAvgBitrate"] << ";\n    "
    when 2
      commandString << "job->vquality = " << hash["VideoQualitySlider"].to_s << ";\n    "
      commandString << "job->crf = 1;\n    "
    end

    #FPS
    if hash["VideoFramerate"] != "Same as source"
      if hash["VideoFramerate"] == "23.976 (NTSC Film)"
        commandString << "job->vrate_base = " << "1126125;\n    "
      elsif hash["VideoFramerate"] == "29.97 (NTSC Video)"
        commandString << "job->vrate_base = " << "900900;\n    "
      elsif hash["VideoFramerate"] == "25 (PAL Film/Video)"
        commandString << "job->vrate_base = " << "1080000\n    "
      # Gotta add the rest of the framerates for completion's sake.
      end
      commandString << "job->cfr = 1;\n    "
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
        when /AC3 /
          audioEncoders << "ac3"
        when /AAC/
          audioEncoders << "faac"
        when /Vorbis/
          audioEncoders << "vorbis"
        when /MP3/
          audioEncoders << "lame"
      end

      #Mixdowns
      case audioTrack["AudioMixdown"]
      when /Mono/
        audioMixdowns << "mono"
      when /Stereo/
        audioMixdowns << "stereo"
      when /Dolby Surround/
        audioMixdowns << "dpl1"
      when /Dolby Pro Logic II/
        audioMixdowns << "dpl2"
      when /discrete/
        audioMixdowns << "6ch"
      when /Passthru/
        audioMixdowns << "auto"
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
    commandString << "    abitrates = strdup(\"" << audioBitrates
    commandString << "\");\n    "
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
    
    #Cropping
    if hash["PictureAutoCrop"] == 0
      commandString << "job->crop[0] = " << hash["PictureTopCrop"].to_s << ";\n    "
      commandString << "job->crop[1] = " << hash["PictureBottomCrop"].to_s << ";\n    "
      commandString << "job->crop[2] = " << hash["PictureLeftCrop"].to_s << ";\n    "
      commandString << "job->crop[4] - " << hash["PictureRightCrop"].to_s << ";\n    "
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
    
    #x264 Options
    if hash["x264Option"] != ""
      commandString << "if( !x264opts )\n    "
      commandString << "{\n    "
      commandString << "    x264opts = strdup(\""
      commandString << hash["x264Option"] << "\");\n    "
      commandString << "}\n    "
    end
    
    #Video Filters
    if hash["UsesPictureFilters"] == 1
      
      case hash["PictureDeinterlace"]
      when 1
        commandString << "deinterlace = 1;\n    "
        commandString << "deinterlace_opt = \"-1\";\n    "
      when 2
        commandString << "deinterlace = 1;\n    "
        commandString << "deinterlace_opt = \"2\";\n    "
      when 3
        commandString << "deinterlace = 1;\n    "
        commandString << "deinterlace_opt = \"0\";\n    "
      when 4
        commandString << "deinterlace = 1;\n    "
        commandString << "deinterlace_opt = \"1:-1:1\";\n    "
      end
      
      case hash["PictureDenoise"]
      when 1
        commandString << "denoise = 1;\n    "
        commandString << "denoise_opt = \"2:1:2:3\";\n    "
      when 2
        commandString << "denoise = 1;\n    "
        commandString << "denoise_opt = \"3:2:2:3\";\n    "
      when 3
        commandString << "denoise = 1;\n    "
        commandString << "denoise_opt = \"7:7:5:5\";\n    "
      end
      
      if hash["PictureDetelecine"] == 1 then commandString << "detelecine = 1;\n    " end
      if hash["PictureDeblock"] == 1 then commandString << "deblock = 1;\n    " end
      if hash["VFR"].to_i == 1 then commandString << "vfr = 1;\n    " end
      if hash["PictureDecomb"] == 1 then commandString << "decomb = 1;\n    " end
      
    end
    
    #Anamorphic
    if hash["PicturePAR"] == 1
      commandString << "anamorphic_mode = 1;\n    "
    elsif hash["PicturePAR"] == 2
      commandString << "anamorphic_mode = 2;\n    "
    end
    
    #Booleans
    if hash["ChapterMarkers"] == 1 then commandString << "job->chapter_markers = 1;\n    " end
    if hash["VideoGrayScale"] == 1 then commandString << "job->grayscale = 1;\n    " end
    if hash["VideoTwoPass"] == 1 then commandString << "twoPass = 1;\n    " end
    if hash["VideoTurboTwoPass"] == 1 then commandString << "turbo_opts_enabled = 1;\n" end
    
    commandString << "}"
    
    # That's it, print to screen now
    puts commandString
    #puts "*" * @columnWidth
    puts  "\n"
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
    commandString << "\\n\");\n\n"    
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
    commandString << "\\n\");\n\n"    
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
    if hash["VideoEncoder"] != "MPEG-4 (FFmpeg)"
      commandString << " -e "
      case hash["VideoEncoder"]
      when /x264/
        commandString << "x264 "
      when /XviD/
        commandString << "xvid "
      end
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
        when /AC3 /
          audioEncoders << "ac3"
        when /AAC/
          audioEncoders << "faac"
        when /Vorbis/
          audioEncoders << "vorbis"
        when /MP3/
          audioEncoders << "lame"
      end
      
      #Mixdowns
      case audioTrack["AudioMixdown"]
      when /Mono/
        audioMixdowns << "mono"
      when /Stereo/
        audioMixdowns << "stereo"
      when /Dolby Surround/
        audioMixdowns << "dpl1"
      when /Dolby Pro Logic II/
        audioMixdowns << "dpl2"
      when /discrete/
        audioMixdowns << "6ch"
      when /Passthru/
        audioMixdowns << "auto"
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
    
    #Container
    commandString << " -f "
    case hash["FileFormat"]
    when /MP4/
      commandString << "mp4"
    when /AVI/
      commandString << "avi"
    when /OGM/
      commandString << "ogm"
    when /MKV/
      commandString << "mkv"
    end
    
    #iPod MP4 atom
    if hash["Mp4iPodCompatible"].to_i == 1
      commandString << " -I"
    end
    
    # 64-bit files
    if hash["Mp4LargeFile"] == 1
      commandString << " -4"
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
    if hash["Subtitles"] != "None"
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
        commandString << " --deinterlace=\\\"fast\\\""
      when 2
        commandString << " --deinterlace=\\\slow\\\""
      when 3
        commandString << " --deinterlace=\\\"slower\\\""
      when 4
        commandString << " --deinterlace=\\\"slowest\\\""
      end
      
      case hash["PictureDenoise"]
      when 1
        commandString << " --denoise=\\\"weak\\\""
      when 2
        commandString << " --denoise=\\\"medium\\\""
      when 3
        commandString << " --denoise=\\\"strong\\\""
      end
      
      if hash["PictureDetelecine"] == 1 then commandString << " --detelecine" end
      if hash["PictureDeblock"] == 1 then commandString << " --deblock" end
      if hash["VFR"].to_i == 1 then commandString << " --vfr" end
      if hash["PictureDecomb"] == 1 then commandString << " --decomb" end
    end
    
    #Anamorphic
    if hash["PicturePAR"] == 1
      commandString << " --strict-anamorphic"
    elsif hash["PicturePAR"] == 2
      commandString << " --loose-anamorphic"
    end
    
    #Booleans
    if hash["ChapterMarkers"] == 1 then commandString << " -m" end
    if hash["VideoGrayScale"] == 1 then commandString << " -g" end
    if hash["VideoTwoPass"] == 1 then commandString << " -2" end
    if hash["VideoTurboTwoPass"] == 1 then commandString << " -T" end
    
      #x264 Options
      if hash["x264Option"] != ""
        commandString << " -x "
        commandString << hash["x264Option"]
      end
    
    commandString << "\\n\");"
    
    # That's it, print to screen now
    puts commandString
    puts  "\n"
  end
  
end

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