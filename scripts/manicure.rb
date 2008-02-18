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
    
    # Grab input from the user's presets .plist
    rawPresets = readPresetPlist
    
    # Store all the presets in here
    presetStew = []

    # Each item in the array is one line from the .plist
    presetStew = rawPresets.split("\n")
    
    # Now get rid of white space
    presetStew = cleanStew(presetStew)
    
    # This stores the offsets between presets.
    presetBreaks = findPresetBreaks(presetStew)

    # Now it's time to use that info to store each
    # preset individually, in the master list.
    @presetMasterList = []
    i = 0
    while i <= presetBreaks.size    
      if i == 0 #first preset
        # Grab the stew, up to the 1st offset.
        @presetMasterList[i] = presetStew.slice(0..presetBreaks[i].to_i)
      elsif i < presetBreaks.size #middle presets
        # Grab the stew from the last offset to the current..
        @presetMasterList[i] = presetStew.slice(presetBreaks[i-1].to_i..presetBreaks[i].to_i)
      else #final preset
        # Grab the stew, starting at the last offset, all the way to the end.
        @presetMasterList[i] = presetStew.slice(presetBreaks[i-1].to_i..presetStew.length)
      end
      i += 1
    end
    
    # Parse the presets into hashes
    @hashMasterList = []
    
    buildPresetHash
    
  end

  def readPresetPlist # Grab the .plist and store it in presets
    
    # Grab the user's home path
    homeLocation = `echo $HOME`.chomp
    
    # Use that to build a path to the presets .plist
    inputFile = homeLocation+'/Library/Application\ Support/HandBrake/UserPresets.plist'
    
    # Builds a command that inputs the .plist, but not before stripping all the XML gobbledygook.
    parseCommand = 'cat '+inputFile+' | sed -e \'s/<[a-z]*>//\' -e \'s/<\/[a-z]*>//\'  -e \'/<[?!]/d\' '
    
    puts "\n\n"
    
    # Run the command, return the raw presets
    rawPresets = `#{parseCommand}`
  end

  def cleanStew(presetStew) #remove tabbed white space
    presetStew.each do |oneline|
      oneline.strip!
    end
  end

  def findPresetBreaks(presetStew) #figure out where each preset starts and ends
    i = 0
    j = 0
    presetBreaks =[]
    presetStew.each do |presetLine|
      if presetLine =~ /AudioBitRate/ # This is the first line of a new preset.
        presetBreaks[j] = i-1         # So mark down how long the last one was.
        j += 1
      end
    i += 1
    end
    return presetBreaks
  end

  def buildPresetHash #fill up @hashMasterList with hashes of all key/value pairs
    j = 0
    
    # Iterate through all presets, treating each in turn as singleServing
    @presetMasterList.each do |singleServing|
      
      # Initialize the hash for preset j (aka singleServing)
      @hashMasterList[j] = Hash.new
      
      # Each key and value are on sequential lines.
      # Iterating through by twos, use that to build a hash.
      # Each key, on line i, paired with its value, on line i+1  
      i = 1
      while i < singleServing.length
        @hashMasterList[j].store( singleServing[i],  singleServing[i+1] )
        i += 2
      end
            
      j += 1  
    end   
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
    
      # Check to make there are valid contents
      if hash.key?("PresetName")
        
        if @options.header == true
          # First throw up a header to make each preset distinct
          displayHeader(hash)
        end
        
        if @options.cliraw == true
          # Show the preset's full CLI string equivalent
          generateCLIString(hash)
        end
        
        if @options.cliparse == true
          generateCLIParse(hash)
        end
        
        if @options.api == true
          # Show the preset as code for test/test.c, HandBrakeCLI
          generateAPIcalls(hash)
        end
        
        if @options.apilist == true
          # Show the preset as print statements, for CLI wrappers to parse.
          generateAPIList(hash) 
        end
      end
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
    if hash["Type"].to_i == 1
      puts "Custom Preset".center(@columnWidth)
    end

    # Note if the preset is marked as default.
    if hash["Default"].to_i == 1
      puts "This is your default preset.".center(@columnWidth)
    end
    
    # End with a line of tildes.  
    puts "~" * @columnWidth
    
  end
  
  def generateCLIString(hash) # Makes a full CLI equivalent of a preset
    commandString = ""
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
    if hash["VideoEncoder"] != "FFmpeg"
      commandString << " -e "
      commandString << hash["VideoEncoder"].to_s.downcase
    end

    #VideoRateControl
    case hash["VideoQualityType"].to_i
    when 0
      commandString << " -S " << hash["VideoTargetSize"]
    when 1
      commandString << " -b " << hash["VideoAvgBitrate"]
    when 2
      commandString << " -q " << hash["VideoQualitySlider"]
    end

    #FPS
    if hash["VideoFramerate"] != "Same as source"
      if hash["VideoFramerate"] == "23.976 (NTSC Film)"
        commandString << " -r " << "23.976"
      elsif hash["VideoFramerate"] == "29.97 (NTSC Video)"
        commandString << " -r " << "29.97"
      else
        commandString << " -r " << hash["VideoFramerate"]
      end
    end

    #Audio encoder (only specifiy bitrate and samplerate when not doing AC-3 pass-thru)
    commandString << " -E "
    case hash["FileCodecs"]
    when /AAC + AC3 Audio/
      commandString << "aac+ac3"
    when /AC-3 /
      commandString << "ac3"
    when /AAC Audio/
      commandString << "faac" << " -B " << hash["AudioBitRate"] << " -R " << hash["AudioSampleRate"]
    when /Vorbis/
      commandString << "vorbis" << " -B " << hash["AudioBitRate"] << " -R " << hash["AudioSampleRate"]
    when /MP3/
      commandString << "lame" << " -B " << hash["AudioBitRate"] << " -R " << hash["AudioSampleRate"]
    end
    
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
    if hash["Mp4LargeFile"].to_i == 1
      commandString << " -4"
    end
    
    #Cropping
    if !hash["PictureAutoCrop"].to_i
      commandString << " --crop "
      commandString << hash["PictureTopCrop"]
      commandString << ":"
      commandString << hash["PictureBottomCrop"]
      commandString << ":"
      commandString << hash["PictureLeftCrop"]
      commandString << ":"
      commandString << hash["PictureRightCrop"]
    end
    
    #Dimensions
    if hash["PictureWidth"].to_i != 0
      commandString << " -w "
      commandString << hash["PictureWidth"]
    end
    if hash["PictureHeight"].to_i != 0
      commandString << " -l "
      commandString << hash["PictureHeight"]
    end
    
    #Subtitles
    if hash["Subtitles"] != "None"
      commandString << " -s "
      commandString << hash["Subtitles"]
    end

    #Video Filters
    if hash["UsesPictureFilters"].to_i == 1
      
      case hash["PictureDeinterlace"].to_i
      when 1
        commandString << " --deinterlace=\"fast\""
      when 2
        commandString << " --deinterlace=\slow\""
      when 3
        commandString << " --deinterlace=\"slower\""
      when 4
        commandString << " --deinterlace=\"slowest\""
      end
      
      case hash["PictureDenoise"].to_i
      when 1
        commandString << " --denoise=\"weak\""
      when 2
        commandString << " --denoise=\"medium\""
      when 3
        commandString << " --denoise=\"strong\""
      end
      
      if hash["PictureDetelecine"].to_i == 1 then commandString << " --detelecine" end
      if hash["PictureDeblock"].to_i == 1 then commandString << " --deblock" end
      if hash["VFR"].to_i == 1 then commandString << " --vfr" end
    end

    #Booleans
    if hash["ChapterMarkers"].to_i == 1 then commandString << " -m" end
    if hash["PicturePAR"].to_i == 1 then commandString << " -p" end
    if hash["VideoGrayScale"].to_i == 1 then commandString << " -g" end
    if hash["VideoTwoPass"].to_i == 1 then commandString << " -2" end
    if hash["VideoTurboTwoPass"].to_i == 1 then commandString << " -T" end

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

  def generateCLIParse(hash) # Makes a CLI equivalent of all user presets, for wrappers to parse
    commandString = ""
    commandString << '+ ' << hash["PresetName"] << ":"
        
    #Video encoder
    if hash["VideoEncoder"] != "FFmpeg"
      commandString << " -e "
      commandString << hash["VideoEncoder"].to_s.downcase
    end

    #VideoRateControl
    case hash["VideoQualityType"].to_i
    when 0
      commandString << " -S " << hash["VideoTargetSize"]
    when 1
      commandString << " -b " << hash["VideoAvgBitrate"]
    when 2
      commandString << " -q " << hash["VideoQualitySlider"]
    end

    #FPS
    if hash["VideoFramerate"] != "Same as source"
      if hash["VideoFramerate"] == "23.976 (NTSC Film)"
        commandString << " -r " << "23.976"
      elsif hash["VideoFramerate"] == "29.97 (NTSC Video)"
        commandString << " -r " << "29.97"
      else
        commandString << " -r " << hash["VideoFramerate"]
      end
    end
    
    #Audio encoder (only include bitrate and samplerate when not doing AC3 passthru)
    commandString << " -E "
    case hash["FileCodecs"]
    when /AC3 Audio/
      commandString << "aac+ac3"
    when /AC-3/
      commandString << "ac3"
    when /AAC Audio/
      commandString << "faac" << " -B " << hash["AudioBitRate"] << " -R " << hash["AudioSampleRate"]
    when /Vorbis/
      commandString << "vorbis" << " -B " << hash["AudioBitRate"] << " -R " << hash["AudioSampleRate"]
    when /MP3/
      commandString << "lame" << " -B " << hash["AudioBitRate"] << " -R " << hash["AudioSampleRate"]
    end
    
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
    if hash["Mp4LargeFile"].to_i == 1
      commandString << " -4"
    end
    
    #Cropping
    if !hash["PictureAutoCrop"].to_i
      commandString << " --crop "
      commandString << hash["PictureTopCrop"]
      commandString << ":"
      commandString << hash["PictureBottomCrop"]
      commandString << ":"
      commandString << hash["PictureLeftCrop"]
      commandString << ":"
      commandString << hash["PictureRightCrop"]
    end
    
    #Dimensions
    if hash["PictureWidth"].to_i != 0
      commandString << " -w "
      commandString << hash["PictureWidth"]
    end
    if hash["PictureHeight"].to_i != 0
      commandString << " -l "
      commandString << hash["PictureHeight"]
    end
    
    #Subtitles
    if hash["Subtitles"] != "None"
      commandString << " -s "
      commandString << hash["Subtitles"]
    end
    
    #Video Filters
    if hash["UsesPictureFilters"].to_i == 1
      
      case hash["PictureDeinterlace"].to_i
      when 1
        commandString << " --deinterlace=\"fast\""
      when 2
        commandString << " --deinterlace=\slow\""
      when 3
        commandString << " --deinterlace=\"slower\""
      when 4
        commandString << " --deinterlace=\"slowest\""
      end
      
      case hash["PictureDenoise"].to_i
      when 1
        commandString << " --denoise=\"weak\""
      when 2
        commandString << " --denoise=\"medium\""
      when 3
        commandString << " --denoise=\"strong\""
      end
      
      if hash["PictureDetelecine"].to_i == 1 then commandString << " --detelecine" end
      if hash["PictureDeblock"].to_i == 1 then commandString << " --deblock" end
      if hash["VFR"].to_i == 1 then commandString << " --vfr" end
    end

    #Booleans
    if hash["ChapterMarkers"].to_i == 1 then commandString << " -m" end
    if hash["PicturePAR"].to_i == 1 then commandString << " -p" end
    if hash["VideoGrayScale"].to_i == 1 then commandString << " -g" end
    if hash["VideoTwoPass"].to_i == 1 then commandString << " -2" end
    if hash["VideoTurboTwoPass"].to_i == 1 then commandString << " -T" end

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
    case hash["FileFormat"]
    when /MP4/
      commandString << "mux = " << "HB_MUX_MP4;\n    "
    when /AVI/
      commandString << "mux = " << "HB_MUX_AVI;\n    "
    when /OGM/
      commandString << "mux = " << "HB_MUX_OGM;\n    "
    when /MKV/
      commandString << "mux = " << "HB_MUX_MKV;\n    "
    end
    
    #iPod MP4 atom
    if hash["Mp4iPodCompatible"].to_i == 1
      commandString << "job->ipod_atom = 1;\n   "
    end
    
    # 64-bit files
    if hash["Mp4LargeFile"].to_i == 1
      commandString << "job->largeFileSize = 1;\n"
    end
    
    #Video encoder
    if hash["VideoEncoder"] != "FFmpeg"
      commandString << "vcodec = "
      if hash["VideoEncoder"] == "x264"
        commandString << "HB_VCODEC_X264;\n    "
      elsif hash["VideoEncoder"].to_s.downcase == "xvid"
        commandString << "HB_VCODEC_XVID;\n    "        
      end
    end

    #VideoRateControl
    case hash["VideoQualityType"].to_i
    when 0
      commandString << "size = " << hash["VideoTargetSize"] << ";\n    "
    when 1
      commandString << "job->vbitrate = " << hash["VideoAvgBitrate"] << ";\n    "
    when 2
      commandString << "job->vquality = " << hash["VideoQualitySlider"] << ";\n    "
      commandString << "job->crf = 1;\n    "
    end

    #FPS
    if hash["VideoFramerate"] != "Same as source"
      if hash["VideoFramerate"] == "23.976 (NTSC Film)"
        commandString << "job->vrate_base = " << "1126125;\n    "
      elsif hash["VideoFramerate"] == "29.97 (NTSC Video)"
        commandString << "job->vrate_base = " << "900900;\n    "
      # Gotta add the rest of the framerates for completion's sake.
      end
    end
    
    # Only include samplerate and bitrate when not performing AC3 passthru
    if (hash["FileCodecs"].include? "AC-3") == false
      #Audio bitrate
      commandString << "job->abitrate = " << hash["AudioBitRate"] << ";\n    "
    
      #Audio samplerate
      commandString << "job->arate = "
      case hash["AudioSampleRate"]
      when /48/
        commandString << "48000"
      when /44.1/
        commandString << "44100"
      when /32/
        commandString << "32000"
      when /24/
        commandString << "24000"
      when /22.05/
        commandString << "22050"
      end
      commandString << ";\n    "
    end
      
    #Audio encoder
    commandString << "acodec = "
    case hash["FileCodecs"]
    when /AC3 Audio/
      commandString << "HB_ACODEC_FAAC;\n    "
      commandString << "audio_mixdown = HB_AMIXDOWN_DOLBYPLII_AC3;\n    "
      commandString << "arate = 48000;\n    "
    when /AAC Audio/
      commandString << "HB_ACODEC_FAAC;\n    "
    when /AC-3/
      commandString << "HB_ACODEC_AC3;\n    "
    when /Vorbis/
      commandString << "HB_ACODEC_VORBIS;\n    "
    when /MP3/
      commandString << "HB_ACODEC_LAME;\n    "
    end
    
    #Cropping
    if !hash["PictureAutoCrop"].to_i
      commandString << "job->crop[0] = " << hash["PictureTopCrop"] << ";\n    "
      commandString << "job->crop[1] = " << hash["PictureBottomCrop"] << ";\n    "
      commandString << "job->crop[2] = " << hash["PictureLeftCrop"] << ";\n    "
      commandString << "job->crop[4] - " << hash["PictureRightCrop"] << ";\n    "
    end
    
    #Dimensions
    if hash["PictureWidth"].to_i != 0
      commandString << "job->width = "
      commandString << hash["PictureWidth"] << ";\n    "
    end
    if hash["PictureHeight"].to_i != 0
      commandString << "job->height = "
      commandString << hash["PictureHeight"] << ";\n    "
    end
    
    #Subtitles
    if hash["Subtitles"] != "None"
      commandString << "job->subtitle = "
      commandString << ( hash["Subtitles"].to_i - 1).to_s << ";\n    "
    end
    
    #x264 Options
    if hash["x264Option"] != ""
      commandString << "x264opts = strdup(\""
      commandString << hash["x264Option"] << "\");\n    "
    end
    
    #Video Filters
    if hash["UsesPictureFilters"].to_i == 1
      
      case hash["PictureDeinterlace"].to_i
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
      
      case hash["PictureDenoise"].to_i
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
      
      if hash["PictureDetelecine"].to_i == 1 then commandString << "detelecine = 1;\n    " end
      if hash["PictureDeblock"].to_i == 1 then commandString << "deblock = 1;\n    " end
      if hash["VFR"].to_i == 1 then commandString << "vfr = 1;\n    " end
    end
    
    #Booleans
    if hash["ChapterMarkers"].to_i == 1 then commandString << "job->chapter_markers = 1;\n    " end
    if hash["PicturePAR"].to_i == 1 then commandString << "pixelratio = 1;\n    " end
    if hash["VideoGrayScale"].to_i == 1 then commandString << "job->grayscale = 1;\n    " end
    if hash["VideoTwoPass"].to_i == 1 then commandString << "twoPass = 1;\n    " end
    if hash["VideoTurboTwoPass"].to_i == 1 then commandString << "turbo_opts_enabled = 1;\n" end
    
    commandString << "}"
    
    # That's it, print to screen now
    puts commandString
    #puts "*" * @columnWidth
    puts  "\n"
  end

  def generateAPIList(hash) # Makes a list of the CLI options a built-in CLI preset uses, for wrappers to parse
    commandString = ""
    commandString << "    printf(\"\\n+ " << hash["PresetName"] << ": "
        
    #Video encoder
    if hash["VideoEncoder"] != "FFmpeg"
      commandString << " -e "
      commandString << hash["VideoEncoder"].to_s.downcase
    end

    #VideoRateControl
    case hash["VideoQualityType"].to_i
    when 0
      commandString << " -S " << hash["VideoTargetSize"]
    when 1
      commandString << " -b " << hash["VideoAvgBitrate"]
    when 2
      commandString << " -q " << hash["VideoQualitySlider"]
    end

    #FPS
    if hash["VideoFramerate"] != "Same as source"
      if hash["VideoFramerate"] == "23.976 (NTSC Film)"
        commandString << " -r " << "23.976"
      elsif hash["VideoFramerate"] == "29.97 (NTSC Video)"
        commandString << " -r " << "29.97"
      else
        commandString << " -r " << hash["VideoFramerate"]
      end
    end
    
    # Only include samplerate and bitrate when not performing AC-3 passthru
    if (hash["FileCodecs"].include? "AC-3") == false
      #Audio bitrate
      commandString << " -B " << hash["AudioBitRate"]
      #Audio samplerate
      commandString << " -R " << hash["AudioSampleRate"]
    end
    
    #Audio encoder
    commandString << " -E "
    case hash["FileCodecs"]
    when /AC3 Audio/
      commandString << "aac+ac3"
    when /AAC Audio/
      commandString << "faac"
    when /AC-3/
      commandString << "ac3"
    when /Vorbis/
      commandString << "vorbis"
    when /MP3/
      commandString << "lame"
    end
    
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
    if hash["Mp4LargeFile"].to_i == 1
      commandString << " -4"
    end
    
    #Cropping
    if !hash["PictureAutoCrop"].to_i
      commandString << " --crop "
      commandString << hash["PictureTopCrop"]
      commandString << ":"
      commandString << hash["PictureBottomCrop"]
      commandString << ":"
      commandString << hash["PictureLeftCrop"]
      commandString << ":"
      commandString << hash["PictureRightCrop"]
    end
    
    #Dimensions
    if hash["PictureWidth"].to_i != 0
      commandString << " -w "
      commandString << hash["PictureWidth"]
    end
    if hash["PictureHeight"].to_i != 0
      commandString << " -l "
      commandString << hash["PictureHeight"]
    end
    
    #Subtitles
    if hash["Subtitles"] != "None"
      commandString << " -s "
      commandString << hash["Subtitles"]
    end
    
    #Video Filters
    if hash["UsesPictureFilters"].to_i == 1
      
      case hash["PictureDeinterlace"].to_i
      when 1
        commandString << " --deinterlace=\\\"fast\\\""
      when 2
        commandString << " --deinterlace=\\\slow\\\""
      when 3
        commandString << " --deinterlace=\\\"slower\\\""
      when 4
        commandString << " --deinterlace=\\\"slowest\\\""
      end
      
      case hash["PictureDenoise"].to_i
      when 1
        commandString << " --denoise=\\\"weak\\\""
      when 2
        commandString << " --denoise=\\\"medium\\\""
      when 3
        commandString << " --denoise=\\\"strong\\\""
      end
      
      if hash["PictureDetelecine"].to_i == 1 then commandString << " --detelecine" end
      if hash["PictureDeblock"].to_i == 1 then commandString << " --deblock" end
      if hash["VFR"].to_i == 1 then commandString << " --vfr" end
    end
    
    #Booleans
    if hash["ChapterMarkers"].to_i == 1 then commandString << " -m" end
    if hash["PicturePAR"].to_i == 1 then commandString << " -p" end
    if hash["VideoGrayScale"].to_i == 1 then commandString << " -g" end
    if hash["VideoTwoPass"].to_i == 1 then commandString << " -2" end
    if hash["VideoTurboTwoPass"].to_i == 1 then commandString << " -T" end
    
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