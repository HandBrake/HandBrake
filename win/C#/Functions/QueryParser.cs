/*  QueryParser.cs $
 	
 	   This file is part of the HandBrake source code.
 	   Homepage: <http://handbrake.fr>.
 	   It may be used under the terms of the GNU General Public License. */

using System;
using System.Globalization;
using System.Text.RegularExpressions;
using System.Windows.Forms;

namespace Handbrake.Functions
{
    internal class QueryParser
    {
        private static readonly CultureInfo Culture = new CultureInfo("en-US", false);

        #region Varibles

        #region Source
        public int DVDTitle { get; private set; }
        public int DVDChapterStart { get; private set; }
        public int DVDChapterFinish { get; private set; }
        #endregion

        #region Output Settings
        public string Format { get; private set; }
        public Boolean LargeMP4 { get; private set; }
        public Boolean IpodAtom { get; private set; }
        public Boolean OptimizeMP4 { get; private set; }
        #endregion

        #region Picture Settings
        public int Width { get; private set; }
        public int Height { get; private set; }
        public int MaxWidth { get; private set; }
        public int MaxHeight { get; private set; }
        public string CropValues { get; private set; }
        public string CropTop { get; private set; }
        public string CropBottom { get; private set; }
        public string CropLeft { get; private set; }
        public string CropRight { get; private set; }
        public Boolean Anamorphic { get; private set; }
        public Boolean LooseAnamorphic { get; private set; }
        #endregion

        #region Video Filters
        public string DeTelecine { get; private set; }
        public int DeBlock { get; private set; }
        public string DeInterlace { get; private set; }
        public string DeNoise { get; private set; }
        public string Decomb { get; private set; }
        #endregion    

        #region Video Settings
        public string VideoEncoder { get; private set; }
        public Boolean Grayscale { get; private set; }
        public Boolean TwoPass { get; private set; }
        public Boolean TurboFirstPass { get; private set; }
        public string VideoFramerate { get; private set; }
        public string AverageVideoBitrate { get; private set; }
        public string VideoTargetSize { get; private set; }
        public float VideoQuality { get; private set; }
        #endregion

        #region Audio Settings
        public string AudioTrack1 { get; private set; }
        public string AudioTrack2 { get; private set; }
        public string AudioTrack3 { get; private set; }
        public string AudioTrack4 { get; private set; }
        public string AudioTrackMix1 { get; private set; }
        public string AudioTrackMix2 { get; private set; }
        public string AudioTrackMix3 { get; private set; }
        public string AudioTrackMix4 { get; private set; }
        public string AudioEncoder1 { get; private set; }
        public string AudioEncoder2 { get; private set; }
        public string AudioEncoder3 { get; private set; }
        public string AudioEncoder4 { get; private set; }
        public string AudioBitrate1 { get; private set; }
        public string AudioBitrate2 { get; private set; }
        public string AudioBitrate3 { get; private set; }
        public string AudioBitrate4 { get; private set; }
        public string AudioSamplerate1 { get; private set; }
        public string AudioSamplerate2 { get; private set; }
        public string AudioSamplerate3 { get; private set; }
        public string AudioSamplerate4 { get; private set; }
        public double DRC1 { get; private set; }
        public double DRC2 { get; private set; }
        public double DRC3 { get; private set; }
        public double DRC4 { get; private set; }
        public string Subtitles { get; private set; }
        public Boolean ForcedSubtitles { get; private set; }
        #endregion

        #region Chapter Markers
        public Boolean ChapterMarkers { get; private set; }
        #endregion

        #region Other
        public string H264Query { get; private set; }
        public Boolean Verbose { get; private set; }
        #endregion

        #endregion

        // All the Main Window GUI options
        /// <summary>
        /// Takes in a query which can be in any order and parses it. 
        /// All varibles are then set so they can be used elsewhere.
        /// </summary>
        /// <param name="input">A ClI Query</param>
        /// <returns>A Parsed Query</returns>
        public static QueryParser Parse(String input)
        {
            var thisQuery = new QueryParser();

            #region Regular Expressions

            // Useful Destination Finder
            //Regex r1 = new Regex(@"(-i)(?:\s\"")([a-zA-Z0-9?';!^%&*()_\-:\\\s\.]+)(?:\"")");
            //Match source = r1.Match(input.Replace('"', '\"'));

            //Source
            Match title = Regex.Match(input, @"-t ([0-9]*)");
            Match chapters = Regex.Match(input, @"-c ([0-9-]*)");
           
            //Output Settings
            Match format = Regex.Match(input, @"-f ([a-z0-9a-z0-9a-z0-9]*)");
            Match grayscale = Regex.Match(input, @" -g");
            Match largerMp4 = Regex.Match(input, @" -4");
            Match ipodAtom = Regex.Match(input, @" -I");

            //Picture Settings Tab
            Match width = Regex.Match(input, @"-w ([0-9]*)");
            Match height = Regex.Match(input, @"-l ([0-9]*)");
            Match maxWidth = Regex.Match(input, @"-X ([0-9]*)");
            Match maxHeight = Regex.Match(input, @"-Y ([0-9]*)");
            Match crop = Regex.Match(input, @"--crop ([0-9]*):([0-9]*):([0-9]*):([0-9]*)");
            Match lanamorphic = Regex.Match(input, @" -P");
            Match anamorphic = Regex.Match(input, @" -p ");

            // Picture Settings - Filters
            Match decomb = Regex.Match(input, @" --decomb");
            Match decombValue = Regex.Match(input, @" --decomb=\""([a-zA-Z0-9.:]*)\""");
            Match deinterlace = Regex.Match(input, @"--deinterlace=\""([a-zA-Z0-9.:]*)\""");
            Match denoise = Regex.Match(input, @"--denoise=\""([a-zA-Z0-9.:]*)\""");
            Match deblock = Regex.Match(input, @"--deblock=([0-9:]*)");
            Match detelecine = Regex.Match(input, @"--detelecine");
            Match detelecineValue = Regex.Match(input, @" --detelecine=\""([a-zA-Z0-9.:]*)\""");

            //Video Settings Tab
            Match videoEncoder = Regex.Match(input, @"-e ([a-zA-Z0-9]*)");
            Match videoFramerate = Regex.Match(input, @"-r ([0-9.]*)");
            Match videoBitrate = Regex.Match(input, @"-b ([0-9]*)");
            Match videoQuality = Regex.Match(input, @"-q ([0-9.]*)");
            Match videoFilesize = Regex.Match(input, @"-S ([0-9.]*)");
            Match twoPass = Regex.Match(input, @" -2");
            Match turboFirstPass = Regex.Match(input, @" -T");
            Match optimizeMP4 = Regex.Match(input, @" -O");

            //Audio Settings Tab
            Match noAudio = Regex.Match(input, @"-a none");
            Match audioTrack1 = Regex.Match(input, @"-a ([0-9]*)");
            Match audioTrack2 = Regex.Match(input, @"-a ([0-9]*),([0-9]*)");
            Match audioTrack3 = Regex.Match(input, @"-a ([0-9]*),([0-9]*),([0-9]*)");
            Match audioTrack4 = Regex.Match(input, @"-a ([0-9]*),([0-9]*),([0-9]*),([0-9]*)");

            Match audioTrack1Mix = Regex.Match(input, @"-6 ([0-9a-z]*)");
            Match audioTrack2Mix = Regex.Match(input, @"-6 ([0-9a-z]*),([0-9a-z]*)");
            Match audioTrack3Mix = Regex.Match(input, @"-6 ([0-9a-z]*),([0-9a-z]*),([0-9a-z]*)");
            Match audioTrack4Mix = Regex.Match(input, @"-6 ([0-9a-z]*),([0-9a-z]*),([0-9a-z]*),([0-9a-z]*)");

            Match audioEncoder1 = Regex.Match(input, @"-E ([a-zA-Z0-9+]*)");
            Match audioEncoder2 = Regex.Match(input, @"-E ([a-zA-Z0-9+]*),([a-zA-Z0-9+]*)");
            Match audioEncoder3 = Regex.Match(input, @"-E ([a-zA-Z0-9+]*),([a-zA-Z0-9+]*),([a-zA-Z0-9+]*)");
            Match audioEncoder4 = Regex.Match(input, @"-E ([a-zA-Z0-9+]*),([a-zA-Z0-9+]*),([a-zA-Z0-9+]*),([a-zA-Z0-9+]*)");

            Match audioBitrate1 = Regex.Match(input, @"-B ([0-9auto]*)");
            Match audioBitrate2 = Regex.Match(input, @"-B ([0-9auto]*),([0-9auto]*)");
            Match audioBitrate3 = Regex.Match(input, @"-B ([0-9auto]*),([0-9auto]*),([0-9auto]*)");
            Match audioBitrate4 = Regex.Match(input, @"-B ([0-9auto]*),([0-9auto]*),([0-9auto]*),([0-9auto]*)");

            Match audioSampleRate1 = Regex.Match(input, @"-R ([0-9Auto.]*)");
            Match audioSampleRate2 = Regex.Match(input, @"-R ([0-9Auto.]*),([0-9Auto.]*)");
            Match audioSampleRate3 = Regex.Match(input, @"-R ([0-9Auto.]*),([0-9Auto.]*),([0-9Auto.]*)");
            Match audioSampleRate4 = Regex.Match(input, @"-R ([0-9Auto.]*),([0-9Auto.]*),([0-9Auto.]*),([0-9Auto.]*)");

            Match drc1 = Regex.Match(input, @"-D ([0-9.]*)");
            Match drc2 = Regex.Match(input, @"-D ([0-9.]*),([0-9.]*)");
            Match drc3 = Regex.Match(input, @"-D ([0-9.]*),([0-9.]*),([0-9.]*)");
            Match drc4 = Regex.Match(input, @"-D ([0-9.]*),([0-9.]*),([0-9.]*),([0-9.]*)");

            Match subtitles = Regex.Match(input, @"-s ([0-9a-zA-Z]*)");
            Match subScan = Regex.Match(input, @" -U");
            Match forcedSubtitles = Regex.Match(input, @" -F");

            // Chapters Tab
            Match chapterMarkers = Regex.Match(input, @" -m");
            Match chapterMarkersFileMode = Regex.Match(input, @"--markers");

            //H264 Tab
            Match x264 = Regex.Match(input, @"-x ([.,/a-zA-Z0-9=:-]*)");

            //Program Options
            Match verbose = Regex.Match(input, @" -v");

            #endregion

            #region Set Varibles

            try
            {
                #region Source Tab

                if (title.Success)
                    thisQuery.DVDTitle = int.Parse(title.ToString().Replace("-t ", ""));

                if (chapters.Success)
                {
                    string[] actTitles = chapters.ToString().Replace("-c ", "").Split('-');
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
                    thisQuery.Format = format.ToString().Replace("-f ", "");
                thisQuery.LargeMP4 = largerMp4.Success;
                thisQuery.IpodAtom = ipodAtom.Success;
                thisQuery.OptimizeMP4 = optimizeMP4.Success;

                #endregion

                #region Picture Tab

                if (width.Success)
                    thisQuery.Width = int.Parse(width.ToString().Replace("-w ", ""));

                if (height.Success)
                    thisQuery.Height = int.Parse(height.ToString().Replace("-l ", ""));

                if (maxWidth.Success)
                    thisQuery.MaxWidth = int.Parse(maxWidth.ToString().Replace("-X ", ""));

                if (maxHeight.Success)
                    thisQuery.MaxHeight = int.Parse(maxHeight.ToString().Replace("-Y ", ""));

                if (crop.Success)
                {
                    thisQuery.CropValues = crop.ToString().Replace("--crop ", "");
                    string[] actCropValues = thisQuery.CropValues.Split(':');
                    thisQuery.CropTop = actCropValues[0];
                    thisQuery.CropBottom = actCropValues[1];
                    thisQuery.CropLeft = actCropValues[2];
                    thisQuery.CropRight = actCropValues[3];
                }
                
                thisQuery.Anamorphic = anamorphic.Success;
                thisQuery.LooseAnamorphic = lanamorphic.Success;

                #endregion

                #region Filters

                thisQuery.Decomb = "Off";
                if (decomb.Success)
                {
                    thisQuery.Decomb = "Default";
                    if (decombValue.Success)
                        thisQuery.Decomb = decombValue.ToString().Replace("--decomb=", "").Replace("\"", "");
                }

                thisQuery.DeInterlace = "None";
                if (deinterlace.Success)
                {
                    thisQuery.DeInterlace = deinterlace.ToString().Replace("--deinterlace=", "").Replace("\"", "");
                    thisQuery.DeInterlace = thisQuery.DeInterlace.Replace("fast", "Fast").Replace("slow", "Slow").Replace("slower", "Slower");
                    thisQuery.DeInterlace = thisQuery.DeInterlace.Replace("slowest", "Slowest");
                }

                thisQuery.DeNoise = "None";
                if (denoise.Success)
                {
                    thisQuery.DeNoise = denoise.ToString().Replace("--denoise=", "").Replace("\"", "");
                    thisQuery.DeNoise = thisQuery.DeNoise.Replace("weak", "Weak").Replace("medium", "Medium").Replace("strong", "Strong");
                }

                string deblockValue = "";
                thisQuery.DeBlock = 0;
                if (deblock.Success)
                    deblockValue = deblock.ToString().Replace("--deblock=", "");

                int dval = 0;
                if (deblockValue != "")
                    int.TryParse(deblockValue, out dval);
                thisQuery.DeBlock = dval;

                thisQuery.DeTelecine = "Off";
                if (detelecine.Success)
                {
                    thisQuery.DeTelecine = "Default";
                    if (detelecineValue.Success)
                        thisQuery.DeTelecine = detelecineValue.ToString().Replace("--detelecine=", "").Replace("\"", "");
                }

                #endregion

                #region Video Settings Tab

                string videoEncoderConvertion = videoEncoder.ToString().Replace("-e ", "");
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
                thisQuery.VideoFramerate = videoFramerate.Success ? videoFramerate.ToString().Replace("-r ", "") : "Same as source";
                thisQuery.Grayscale = grayscale.Success;
                thisQuery.TwoPass = twoPass.Success;
                thisQuery.TurboFirstPass = turboFirstPass.Success;
                
                if (videoBitrate.Success)
                    thisQuery.AverageVideoBitrate = videoBitrate.ToString().Replace("-b ", "");
                if (videoFilesize.Success)
                    thisQuery.VideoTargetSize = videoFilesize.ToString().Replace("-S ", "");

                if (videoQuality.Success)
                {
                   float qConvert = float.Parse(videoQuality.ToString().Replace("-q ", ""), Culture);
                    //qConvert = Math.Ceiling(qConvert);
                    thisQuery.VideoQuality = qConvert;
                }
                #endregion

                #region Audio Tab

                // Tracks
                if (noAudio.Success)
                    thisQuery.AudioTrack1 = "None";
                else if (audioTrack1.Success)
                    thisQuery.AudioTrack1 = "Automatic";

                if (audioTrack2.Success)
                {
                    string[] audioChan = audioTrack2.ToString().Split(',');
                    thisQuery.AudioTrack2 = audioChan[1];
                }
                else
                    thisQuery.AudioTrack2 = "None";

                if (audioTrack3.Success)
                {
                    string[] audioChan = audioTrack3.ToString().Split(',');
                    thisQuery.AudioTrack3 = audioChan[2];
                }
                else
                    thisQuery.AudioTrack3 = "None";

                if (audioTrack4.Success)
                {
                    string[] audioChan = audioTrack4.ToString().Split(',');
                    thisQuery.AudioTrack4 = audioChan[3];
                }
                else
                    thisQuery.AudioTrack4 = "None";


                // Mixdowns
                thisQuery.AudioTrackMix1 = "Automatic";
                if (audioTrack1Mix.Success)
                    thisQuery.AudioTrackMix1 =
                        getMixDown(audioTrack1Mix.ToString().Replace("-6 ", "").Replace(" ", ""));

                thisQuery.AudioTrackMix2 = "Automatic";
                if (audioTrack2Mix.Success)
                {
                    string[] audio2mix = audioTrack2Mix.ToString().Split(',');
                    thisQuery.AudioTrackMix2 = getMixDown(audio2mix[1].Trim());
                }

                thisQuery.AudioTrackMix3 = "Automatic";
                if (audioTrack3Mix.Success)
                {
                    string[] audio3mix = audioTrack3Mix.ToString().Split(',');
                    thisQuery.AudioTrackMix3 = getMixDown(audio3mix[2].Trim());
                }

                thisQuery.AudioTrackMix4 = "Automatic";
                if (audioTrack4Mix.Success)
                {
                    string[] audio4mix = audioTrack4Mix.ToString().Split(',');
                    thisQuery.AudioTrackMix4 = getMixDown(audio4mix[3].Trim());
                }


                // Audio Encoders
                if (audioEncoder1.Success)
                    thisQuery.AudioEncoder1 = getAudioEncoder(audioEncoder1.ToString().Replace("-E ", ""));

                if (audioEncoder2.Success)
                {
                    string[] audio2enc = audioEncoder2.ToString().Split(',');
                    thisQuery.AudioEncoder2 = getAudioEncoder(audio2enc[1].Trim());
                }

                if (audioEncoder3.Success)
                {
                    string[] audio3enc = audioEncoder3.ToString().Split(',');
                    thisQuery.AudioEncoder3 = getAudioEncoder(audio3enc[2].Trim());
                }

                if (audioEncoder4.Success)
                {
                    string[] audio4enc = audioEncoder4.ToString().Split(',');
                    thisQuery.AudioEncoder4 = getAudioEncoder(audio4enc[3].Trim());
                }


                // Audio Bitrate
                thisQuery.AudioBitrate1 = "";
                if (audioBitrate1.Success)
                {
                    thisQuery.AudioBitrate1 = audioBitrate1.ToString().Replace("-B ", "").Trim();
                    if (audioBitrate1.ToString().Replace("-B ", "").Trim() == "0") thisQuery.AudioBitrate1 = "Auto";
                }

                thisQuery.AudioBitrate2 = "";
                if (audioBitrate2.Success && audioTrack2.Success)
                {
                    string[] audioBitrateSelect = audioBitrate2.ToString().Split(',');
                    if (audioBitrateSelect[1].Trim() == "0") audioBitrateSelect[1] = "Auto";
                    thisQuery.AudioBitrate2 = audioBitrateSelect[1].Trim();
                }

                thisQuery.AudioBitrate3 = "";
                if (audioBitrate3.Success && audioTrack3.Success)
                {
                    string[] audioBitrateSelect = audioBitrate3.ToString().Split(',');
                    if (audioBitrateSelect[2].Trim() == "0") audioBitrateSelect[2] = "Auto";
                    thisQuery.AudioBitrate3 = audioBitrateSelect[2].Trim();
                }

                thisQuery.AudioBitrate4 = "";
                if (audioBitrate4.Success)
                {
                    string[] audioBitrateSelect = audioBitrate4.ToString().Split(',');
                    if (audioBitrateSelect[3].Trim() == "0") audioBitrateSelect[3] = "Auto";
                    thisQuery.AudioBitrate4 = audioBitrateSelect[3].Trim();
                }


                // Audio Sample Rate
                // Make sure to change 0 to Auto
                thisQuery.AudioSamplerate1 = "Auto";
                if (audioSampleRate1.Success)
                {
                    thisQuery.AudioSamplerate1 = audioSampleRate1.ToString().Replace("-R ", "").Trim();
                    if (thisQuery.AudioSamplerate1 == "0") thisQuery.AudioSamplerate1 = "Auto";
                }


                if (audioSampleRate2.Success)
                {
                    string[] audioSRSelect = audioSampleRate2.ToString().Split(',');
                    if (audioSRSelect[1] == "0") audioSRSelect[1] = "Auto";
                    thisQuery.AudioSamplerate2 = audioSRSelect[1].Trim();
                }

                if (audioSampleRate3.Success)
                {
                    string[] audioSRSelect = audioSampleRate3.ToString().Split(',');
                    if (audioSRSelect[2] == "0") audioSRSelect[2] = "Auto";
                    thisQuery.AudioSamplerate3 = audioSRSelect[2].Trim();
                }

                if (audioSampleRate4.Success)
                {
                    string[] audioSRSelect = audioSampleRate4.ToString().Split(',');
                    if (audioSRSelect[3] == "0") audioSRSelect[3] = "Auto";
                    thisQuery.AudioSamplerate4 = audioSRSelect[3].Trim();
                }

                // DRC
                float drcValue;

                thisQuery.DRC1 = 1;
                if (drc1.Success)
                {
                    string value = drc1.ToString().Replace("-D ", "");
                    float.TryParse(value, out drcValue);
                    thisQuery.DRC1 = drcValue;
                }

                thisQuery.DRC2 = 1;
                if (drc2.Success)
                {
                    string[] drcPoint = drc2.ToString().Split(',');
                    float.TryParse(drcPoint[1], out drcValue);
                    thisQuery.DRC2 = drcValue;
                }

                thisQuery.DRC3 = 1;
                if (drc3.Success)
                {
                    string[] drcPoint = drc3.ToString().Split(',');
                    float.TryParse(drcPoint[2], out drcValue);
                    thisQuery.DRC3 = drcValue;
                }

                thisQuery.DRC4 = 1;
                if (drc4.Success)
                {
                    string[] drcPoint = drc4.ToString().Split(',');
                    float.TryParse(drcPoint[3], out drcValue);
                    thisQuery.DRC4 = drcValue;
                }

                // Subtitle Stuff
                if (subtitles.Success)
                    thisQuery.Subtitles = subtitles.ToString().Replace("-s ", "");
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
                    thisQuery.H264Query = x264.ToString().Replace("-x ", "");

                thisQuery.Verbose = verbose.Success;

                #endregion
            }
            catch (Exception exc)
            {
                MessageBox.Show(
                    "An error has occured in the Query Parser. Please report this error on the forum in the 'Windows' support section. \n\n" +
                    exc, "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }

            #endregion

            return thisQuery;
        }

        private static string getMixDown(string mixdown)
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
        private static string getAudioEncoder(string audioEnc)
        {
            switch (audioEnc)
            {
                case "faac":
                    return "AAC";
                case "lame":
                    return "MP3";
                case "vorbis":
                    return "Vorbis";
                case "ac3":
                    return "AC3";
                default:
                    return "AAC";
            }
        }
    }
}