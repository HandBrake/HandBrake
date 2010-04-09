/*  QueryParser.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

namespace Handbrake.Functions
{
    using System;
    using System.Collections;
    using System.Globalization;
    using System.Text.RegularExpressions;
    using System.Windows.Forms;
    using Model;

    /// <summary>
    /// Parse a CLI Query
    /// </summary>
    public class QueryParser
    {
        /// <summary>
        /// The Culture
        /// </summary>
        private static readonly CultureInfo Culture = new CultureInfo("en-US", false);

        #region Varibles

        // Source
        public int DVDTitle { get; set; }
        public int DVDChapterStart { get; set; }
        public int DVDChapterFinish { get; set; }

        // Output Settings
        public string Format { get; set; }
        public bool LargeMP4 { get; set; }
        public bool IpodAtom { get; set; }
        public bool OptimizeMP4 { get; set; }

        // Picture Settings
        public int Width { get; set; }
        public int Height { get; set; }
        public int MaxWidth { get; set; }
        public int MaxHeight { get; set; }
        public string CropValues { get; set; }
        public string CropTop { get; set; }
        public string CropBottom { get; set; }
        public string CropLeft { get; set; }
        public string CropRight { get; set; }
        public int AnamorphicMode { get; set; }
        public bool KeepDisplayAsect { get; set; }
        public double DisplayWidthValue { get; set; }
        public int PixelAspectWidth { get; set; }
        public int PixelAspectHeight { get; set; }
        public int AnamorphicModulus { get; set; }

        // Video Filters
        public string DeTelecine { get; set; }
        public int DeBlock { get; set; }
        public string DeInterlace { get; set; }
        public string DeNoise { get; set; }
        public string Decomb { get; set; }

        // Video Settings
        public string VideoEncoder { get; set; }
        public bool Grayscale { get; set; }
        public bool TwoPass { get; set; }
        public bool TurboFirstPass { get; set; }
        public string VideoFramerate { get; set; }
        public string AverageVideoBitrate { get; set; }
        public string VideoTargetSize { get; set; }
        public float VideoQuality { get; set; }

        // Audio Settings
        public ArrayList AudioInformation { get; set; }
        public string Subtitles { get; set; }
        public bool ForcedSubtitles { get; set; }

        // Chapter Markers
        public bool ChapterMarkers { get; set; }

        // Other
        public string H264Query { get; set; }
        public bool Verbose { get; set; }

        // Preset Information
        public int PresetBuildNumber { get; set; }
        public string PresetDescription { get; set; }
        public string PresetName { get; set; }
        public string Type { get; set; }
        public bool UsesMaxPictureSettings { get; set; }
        public bool UsesPictureFilters { get; set; }
        public bool UsesPictureSettings { get; set; }

        #endregion

        /// <summary>
        /// Takes in a query which can be in any order and parses it. 
        /// All varibles are then set so they can be used elsewhere.
        /// </summary>
        /// <param name="input">A ClI Query</param>
        /// <returns>A Parsed Query</returns>
        public static QueryParser Parse(string input)
        {
            var thisQuery = new QueryParser();

            #region Regular Expressions

            // Source
            Match title = Regex.Match(input, @"-t ([0-9]*)");
            Match chapters = Regex.Match(input, @"-c ([0-9-]*)");

            // Output Settings
            Match format = Regex.Match(input, @"-f ([a-z0-9a-z0-9a-z0-9]*)");
            Match grayscale = Regex.Match(input, @" -g");
            Match largerMp4 = Regex.Match(input, @" -4");
            Match ipodAtom = Regex.Match(input, @" -I");

            // Picture Settings Tab
            Match width = Regex.Match(input, @"-w ([0-9]*)");
            Match height = Regex.Match(input, @"-l ([0-9]*)");
            Match maxWidth = Regex.Match(input, @"-X ([0-9]*)");
            Match maxHeight = Regex.Match(input, @"-Y ([0-9]*)");
            Match crop = Regex.Match(input, @"--crop ([0-9]*):([0-9]*):([0-9]*):([0-9]*)");

            Match looseAnamorphic = Regex.Match(input, @"--loose-anamorphic");
            Match strictAnamorphic = Regex.Match(input, @"--strict-anamorphic");
            Match customAnamorphic = Regex.Match(input, @"--custom-anamorphic");

            Match keepDisplayAsect = Regex.Match(input, @"--keep-display-aspect");
            Match displayWidth = Regex.Match(input, @"--display-width ([0-9]*)");
            Match pixelAspect = Regex.Match(input, @"--pixel-aspect ([0-9]*):([0-9]*)");
            Match modulus = Regex.Match(input, @"--modulus ([0-9]*)");

            // Picture Settings - Filters
            Match decomb = Regex.Match(input, @" --decomb");
            Match decombValue = Regex.Match(input, @" --decomb=\""([a-zA-Z0-9.:]*)\""");
            Match deinterlace = Regex.Match(input, @"--deinterlace=\""([a-zA-Z0-9.:]*)\""");
            Match denoise = Regex.Match(input, @"--denoise=\""([a-zA-Z0-9.:]*)\""");
            Match deblock = Regex.Match(input, @"--deblock=([0-9:]*)");
            Match detelecine = Regex.Match(input, @"--detelecine");
            Match detelecineValue = Regex.Match(input, @" --detelecine=\""([a-zA-Z0-9.:]*)\""");

            // Video Settings Tab
            Match videoEncoder = Regex.Match(input, @"-e ([a-zA-Z0-9]*)");
            Match videoFramerate = Regex.Match(input, @"-r ([0-9.]*)");
            Match videoBitrate = Regex.Match(input, @"-b ([0-9]*)");
            Match videoQuality = Regex.Match(input, @"-q ([0-9.]*)");
            Match videoFilesize = Regex.Match(input, @"-S ([0-9.]*)");
            Match twoPass = Regex.Match(input, @" -2");
            Match turboFirstPass = Regex.Match(input, @" -T");
            Match optimizeMP4 = Regex.Match(input, @" -O");

            // Audio Settings Tab
            Match noAudio = Regex.Match(input, @"-a none");
            Match audioTracks = Regex.Match(input, @"-a ([0-9,]*)");
            Match audioTrackMixes = Regex.Match(input, @"-6 ([0-9a-zA-Z,]*)");
            Match audioEncoders = Regex.Match(input, @"-E ([a-zA-Z0-9+,]*)");
            Match audioBitrates = Regex.Match(input, @"-B ([0-9a-zA-Z,]*)"); // Auto = a-z
            Match audioSampleRates = Regex.Match(input, @"-R ([0-9a-zA-Z.,]*)"); // Auto = a-z
            Match drcValues = Regex.Match(input, @"-D ([0-9.,]*)");

            Match subtitles = Regex.Match(input, @"-s ([0-9a-zA-Z]*)");
            Match subScan = Regex.Match(input, @" -U");
            Match forcedSubtitles = Regex.Match(input, @" -F");

            // Chapters Tab
            Match chapterMarkers = Regex.Match(input, @" -m");
            Match chapterMarkersFileMode = Regex.Match(input, @"--markers");

            // H264 Tab
            Match x264 = Regex.Match(input, @"-x ([.,/a-zA-Z0-9=:-]*)");

            // Program Options
            Match verbose = Regex.Match(input, @" -v");

            #endregion

            #region Set Varibles

            try
            {
                #region Source Tab

                if (title.Success)
                    thisQuery.DVDTitle = int.Parse(title.ToString().Replace("-t ", string.Empty));

                if (chapters.Success)
                {
                    string[] actTitles = chapters.ToString().Replace("-c ", string.Empty).Split('-');
                    thisQuery.DVDChapterStart = int.Parse(actTitles[0]);
                    if (actTitles.Length > 1)
                    {
                        thisQuery.DVDChapterFinish = int.Parse(actTitles[1]);
                    }

                    if ((thisQuery.DVDChapterStart == 1) && (thisQuery.DVDChapterFinish == 0))
                        thisQuery.DVDChapterFinish = thisQuery.DVDChapterStart;
                }

                #endregion

                #region Output Settings

                if (format.Success)
                    thisQuery.Format = format.ToString().Replace("-f ", string.Empty);
                thisQuery.LargeMP4 = largerMp4.Success;
                thisQuery.IpodAtom = ipodAtom.Success;
                thisQuery.OptimizeMP4 = optimizeMP4.Success;

                #endregion

                #region Picture Tab

                if (width.Success)
                    thisQuery.Width = int.Parse(width.Groups[0].Value.Replace("-w ", string.Empty));

                if (height.Success)
                    thisQuery.Height = int.Parse(height.Groups[0].Value.Replace("-l ", string.Empty));

                if (maxWidth.Success)
                    thisQuery.MaxWidth = int.Parse(maxWidth.Groups[0].Value.Replace("-X ", string.Empty));

                if (maxHeight.Success)
                    thisQuery.MaxHeight = int.Parse(maxHeight.Groups[0].Value.Replace("-Y ", string.Empty));

                if (crop.Success)
                {
                    thisQuery.CropValues = crop.ToString().Replace("--crop ", string.Empty);
                    string[] actCropValues = thisQuery.CropValues.Split(':');
                    thisQuery.CropTop = actCropValues[0];
                    thisQuery.CropBottom = actCropValues[1];
                    thisQuery.CropLeft = actCropValues[2];
                    thisQuery.CropRight = actCropValues[3];
                }

                if (strictAnamorphic.Success)
                    thisQuery.AnamorphicMode = 1;
                else if (looseAnamorphic.Success)
                    thisQuery.AnamorphicMode = 2;
                else if (customAnamorphic.Success)
                    thisQuery.AnamorphicMode = 3;
                else
                    thisQuery.AnamorphicMode = 0;

                thisQuery.KeepDisplayAsect = keepDisplayAsect.Success;

                if (displayWidth.Success)
                    thisQuery.DisplayWidthValue =
                        double.Parse(displayWidth.Groups[0].Value.Replace("--display-width ", string.Empty));

                if (pixelAspect.Success)
                    thisQuery.PixelAspectWidth = int.Parse(pixelAspect.Groups[1].Value.Replace("--pixel-aspect ", string.Empty));

                if (pixelAspect.Success && pixelAspect.Groups.Count >= 3)
                    thisQuery.PixelAspectHeight = int.Parse(pixelAspect.Groups[2].Value.Replace("--pixel-aspect ", string.Empty));

                if (modulus.Success)
                    thisQuery.AnamorphicModulus = int.Parse(modulus.Groups[0].Value.Replace("--modulus ", string.Empty));

                #endregion

                #region Filters

                thisQuery.Decomb = "Off";
                if (decomb.Success)
                {
                    thisQuery.Decomb = "Default";
                    if (decombValue.Success)
                        thisQuery.Decomb = decombValue.ToString().Replace("--decomb=", string.Empty).Replace("\"", string.Empty);
                }

                thisQuery.DeInterlace = "Off";
                if (deinterlace.Success)
                {
                    thisQuery.DeInterlace = deinterlace.ToString().Replace("--deinterlace=", string.Empty).Replace("\"", string.Empty);
                    thisQuery.DeInterlace =
                        thisQuery.DeInterlace.Replace("fast", "Fast").Replace("slow", "Slow").Replace("slower", "Slower");
                    thisQuery.DeInterlace = thisQuery.DeInterlace.Replace("slowest", "Slowest");
                }

                thisQuery.DeNoise = "Off";
                if (denoise.Success)
                {
                    thisQuery.DeNoise = denoise.ToString().Replace("--denoise=", string.Empty).Replace("\"", string.Empty);
                    thisQuery.DeNoise =
                        thisQuery.DeNoise.Replace("weak", "Weak").Replace("medium", "Medium").Replace("strong", "Strong");
                }

                string deblockValue = string.Empty;
                thisQuery.DeBlock = 0;
                if (deblock.Success)
                    deblockValue = deblock.ToString().Replace("--deblock=", string.Empty);

                int dval = 0;
                if (deblockValue != string.Empty)
                    int.TryParse(deblockValue, out dval);
                thisQuery.DeBlock = dval;

                thisQuery.DeTelecine = "Off";
                if (detelecine.Success)
                {
                    thisQuery.DeTelecine = "Default";
                    if (detelecineValue.Success)
                        thisQuery.DeTelecine = detelecineValue.ToString().Replace("--detelecine=", string.Empty).Replace("\"", string.Empty);
                }

                #endregion

                #region Video Settings Tab

                string videoEncoderConvertion = videoEncoder.ToString().Replace("-e ", string.Empty);
                switch (videoEncoderConvertion)
                {
                    case "ffmpeg":
                        videoEncoderConvertion = "MPEG-4 (FFmpeg)";
                        break;
                    case "x264":
                        videoEncoderConvertion = "H.264 (x264)";
                        break;
                    case "theora":
                        videoEncoderConvertion = "VP3 (Theora)";
                        break;
                    default:
                        videoEncoderConvertion = "MPEG-4 (FFmpeg)";
                        break;
                }
                thisQuery.VideoEncoder = videoEncoderConvertion;
                thisQuery.VideoFramerate = videoFramerate.Success
                                               ? videoFramerate.ToString().Replace("-r ", string.Empty)
                                               : "Same as source";
                thisQuery.Grayscale = grayscale.Success;
                thisQuery.TwoPass = twoPass.Success;
                thisQuery.TurboFirstPass = turboFirstPass.Success;

                if (videoBitrate.Success)
                    thisQuery.AverageVideoBitrate = videoBitrate.ToString().Replace("-b ", string.Empty);
                if (videoFilesize.Success)
                    thisQuery.VideoTargetSize = videoFilesize.ToString().Replace("-S ", string.Empty);

                if (videoQuality.Success)
                {
                    float qConvert = float.Parse(videoQuality.ToString().Replace("-q ", string.Empty), Culture);
                    thisQuery.VideoQuality = qConvert;
                }
                else
                    thisQuery.VideoQuality = -1;

                #endregion

                #region Audio Tab

                // Find out how many tracks we need to add by checking how many encoders or audio tracks are selected.
                int encoderCount = 0;
                if (audioEncoders.Success)
                {
                    string[] audioDataCounters = audioEncoders.ToString().Replace("-E ", string.Empty).Split(',');
                    encoderCount = audioDataCounters.Length;
                }

                // Get the data from the regular expression results
                string[] trackData = null;
                string[] trackMixes = null;
                string[] trackEncoders = null;
                string[] trackBitrates = null;
                string[] trackSamplerates = null;
                string[] trackDRCvalues = null;

                if (audioTracks.Success)
                    trackData = audioTracks.ToString().Replace("-a ", string.Empty).Split(',');
                if (audioTrackMixes.Success)
                    trackMixes = audioTrackMixes.ToString().Replace("-6 ", string.Empty).Split(',');
                if (audioEncoders.Success)
                    trackEncoders = audioEncoders.ToString().Replace("-E ", string.Empty).Split(',');
                if (audioBitrates.Success)
                    trackBitrates = audioBitrates.ToString().Replace("-B ", string.Empty).Split(',');
                if (audioSampleRates.Success)
                    trackSamplerates = audioSampleRates.ToString().Replace("-R ", string.Empty).Split(',');
                if (drcValues.Success)
                    trackDRCvalues = drcValues.ToString().Replace("-D ", string.Empty).Split(',');

                // Create new Audio Track Classes and store them in the ArrayList
                ArrayList allAudioTrackInfo = new ArrayList();
                for (int x = 0; x < encoderCount; x++)
                {
                    AudioTrack track = new AudioTrack();
                    if (trackData != null)
                        if (trackData.Length >= (x + 1)) // Audio Track
                            track.Track = trackData[x].Trim();

                    if (trackMixes != null)
                        if (trackMixes.Length >= (x + 1)) // Audio Mix
                            track.MixDown = GetMixDown(trackMixes[x].Trim());

                    if (trackEncoders != null)
                        if (trackEncoders.Length >= (x + 1)) // Audio Mix
                            track.Encoder = GetAudioEncoder(trackEncoders[x].Trim());

                    if (trackBitrates != null)
                        if (trackBitrates.Length >= (x + 1)) // Audio Encoder
                            track.Bitrate = trackBitrates[x].Trim() == "auto" ? "Auto" : trackBitrates[x].Trim();

                    if (trackSamplerates != null)
                        if (trackSamplerates.Length >= (x + 1)) // Audio SampleRate
                            track.SampleRate = trackSamplerates[x].Trim() == "0" ? "Auto" : trackSamplerates[x].Trim();

                    if (trackDRCvalues != null)
                        if (trackDRCvalues.Length >= (x + 1)) // Audio DRC Values
                            track.DRC = trackDRCvalues[x].Trim();

                    allAudioTrackInfo.Add(track);
                }
                thisQuery.AudioInformation = allAudioTrackInfo;

                // Subtitle Stuff
                if (subtitles.Success)
                    thisQuery.Subtitles = subtitles.ToString().Replace("-s ", string.Empty);
                else
                    thisQuery.Subtitles = subScan.Success ? "Autoselect" : "None";

                thisQuery.ForcedSubtitles = forcedSubtitles.Success;

                #endregion

                #region Chapters Tab

                if (chapterMarkersFileMode.Success || chapterMarkers.Success)
                    thisQuery.ChapterMarkers = true;

                #endregion

                #region H.264 and other

                if (x264.Success)
                    thisQuery.H264Query = x264.ToString().Replace("-x ", string.Empty);

                thisQuery.Verbose = verbose.Success;

                #endregion
            }
            catch (Exception exc)
            {
                MessageBox.Show("An error has occured in the Query Parser.\n\n" + exc, "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }

            #endregion

            return thisQuery;
        }

        /// <summary>
        /// Get the GUI equiv to a CLI mixdown
        /// </summary>
        /// <param name="mixdown">The Audio Mixdown</param>
        /// <returns>The GUI representation of the mixdown</returns>
        private static string GetMixDown(string mixdown)
        {
            switch (mixdown.Trim())
            {
                case "mono":
                    return "Mono";
                case "stereo":
                    return "Stereo";
                case "dpl1":
                    return "Dolby Surround";
                case "dpl2":
                    return "Dolby Pro Logic II";
                case "6ch":
                    return "6 Channel Discrete";
                default:
                    return "Automatic";
            }
        }

        /// <summary>
        /// Get the GUI equiv to a CLI audio encoder
        /// </summary>
        /// <param name="audioEnc">The Audio Encoder</param>
        /// <returns>The GUI representation of that audio encoder</returns>
        private static string GetAudioEncoder(string audioEnc)
        {
            switch (audioEnc)
            {
                case "faac":
                    return "AAC (faac)";
                case "lame":
                    return "MP3 (lame)";
                case "vorbis":
                    return "Vorbis (vorbis)";
                case "ac3":
                    return "AC3 Passthru";
                case "dts":
                    return "DTS Passthru";
                default:
                    return "AAC (faac)";
            }
        }
    }
}