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

        private int q_chaptersFinish;
        private int q_chaptersStart;
        private int q_dvdTitle;

        /// <summary>
        /// Returns an Integer
        /// DVD Title number.
        /// </summary>
        public int DVDTitle
        {
            get { return q_dvdTitle; }
        }

        /// <summary>
        /// Returns an Int
        /// DVD Chapter number or chapter range.
        /// </summary>
        public int DVDChapterStart
        {
            get { return q_chaptersStart; }
        }

        /// <summary>
        /// Returns an Int
        /// DVD Chapter number or chapter range.
        /// </summary>
        public int DVDChapterFinish
        {
            get { return q_chaptersFinish; }
        }

        #endregion

        #region Destination

        private string q_format;
        private string q_videoEncoder;

        /// <summary>
        /// Returns a String 
        /// Full path of the destination.
        /// </summary>
        public string Format
        {
            get { return q_format; }
        }

        /// <summary>
        /// Returns an String
        /// The Video Encoder used.
        /// </summary>
        public string VideoEncoder
        {
            get { return q_videoEncoder; }
        }

        #endregion

        #region Picture Settings

        private Boolean q_anamorphic;
        private Boolean q_chapterMarkers;
        private string q_cropbottom;
        private string q_cropLeft;
        private string q_cropRight;
        private string q_croptop;
        private string q_cropValues;
        private int q_deBlock;
        private string q_decomb;
        private string q_deinterlace;
        private string q_denoise;
        private string q_detelecine;
        private Boolean q_looseAnamorphic;
        private int q_maxHeight;
        private int q_maxWidth;
        private int q_videoHeight;
        private int q_videoWidth;

        /// <summary>
        /// Returns an Int
        /// The selected Width for the encoding.
        /// </summary>
        public int Width
        {
            get { return q_videoWidth; }
        }

        /// <summary>
        /// Returns an Int
        /// The selected Height for the encoding.
        /// </summary>
        public int Height
        {
            get { return q_videoHeight; }
        }

        /// <summary>
        /// Returns an Int
        /// The selected Width for the encoding.
        /// </summary>
        public int MaxWidth
        {
            get { return q_maxWidth; }
        }

        /// <summary>
        /// Returns an Int
        /// The selected Height for the encoding.
        /// </summary>
        public int MaxHeight
        {
            get { return q_maxHeight; }
        }

        /// <summary>
        /// Returns an String
        /// Cropping values.
        /// </summary>
        public string CropValues
        {
            get { return q_cropValues; }
        }

        /// <summary>
        /// Returns an String
        /// Cropping values.
        /// </summary>
        public string CropTop
        {
            get { return q_croptop; }
        }

        /// <summary>
        /// Returns an String
        /// Cropping values.
        /// </summary>
        public string CropBottom
        {
            get { return q_cropbottom; }
        }

        /// <summary>
        /// Returns an String
        /// Cropping values.
        /// </summary>
        public string CropLeft
        {
            get { return q_cropLeft; }
        }

        /// <summary>
        /// Returns an String
        /// Cropping values.
        /// </summary>
        public string CropRight
        {
            get { return q_cropRight; }
        }

        /// <summary>
        /// Returns a boolean to indicate wither DeTelecine is on or off
        /// </summary>
        public string DeTelecine
        {
            get { return q_detelecine; }
        }

        /// <summary>
        /// Returns a boolean to indicate wither DeBlock is on or off.
        /// </summary>
        public int DeBlock
        {
            get { return q_deBlock; }
        }

        /// <summary>
        /// Returns a string with the De-Interlace option used.
        /// </summary>
        public string DeInterlace
        {
            get { return q_deinterlace; }
        }

        /// <summary>
        /// Returns a string with the DeNoise option used.
        /// </summary>
        public string DeNoise
        {
            get { return q_denoise; }
        }

        /// <summary>
        /// Returns a string with the DeNoise option used.
        /// </summary>
        public string Decomb
        {
            get { return q_decomb; }
        }

        /// <summary>
        /// Returns a boolean to indicate wither Anamorphic is on or off.
        /// </summary>
        public Boolean Anamorphic
        {
            get { return q_anamorphic; }
        }

        /// <summary>
        /// Returns a boolean to indicate wither Anamorphic is on or off.
        /// </summary>
        public Boolean LooseAnamorphic
        {
            get { return q_looseAnamorphic; }
        }

        /// <summary>
        /// Returns a boolean to indicate wither Chapter Markers is on or off.
        /// </summary>
        public Boolean ChapterMarkers
        {
            get { return q_chapterMarkers; }
        }

        #endregion

        #region Video Settings

        private string q_avgBitrate;
        private Boolean q_grayscale;
        private Boolean q_ipodAtom;
        private Boolean q_largeMp4;
        private Boolean q_optimizeMp4;
        private Boolean q_turboFirst;

        private Boolean q_twoPass;
        private string q_videoFramerate;
        private float q_videoQuality;
        private string q_videoTargetSize;

        /// <summary>
        /// Returns a boolean to indicate wither Grayscale is on or off.
        /// </summary>
        public Boolean Grayscale
        {
            get { return q_grayscale; }
        }

        /// <summary>
        /// Returns a boolean to indicate wither Two Pass Encoding is on or off.
        /// </summary>
        public Boolean TwoPass
        {
            get { return q_twoPass; }
        }

        /// <summary>
        /// Returns a boolean to indicate wither Chapter Markers is on or off.
        /// </summary>
        public Boolean TurboFirstPass
        {
            get { return q_turboFirst; }
        }

        /// <summary>
        /// Returns a boolean to indicate wither Larger MP4 files is on or off.
        /// </summary>
        public Boolean LargeMP4
        {
            get { return q_largeMp4; }
        }

        /// <summary>
        /// Returns a boolean to indicate wither Larger MP4 files is on or off.
        /// </summary>
        public Boolean IpodAtom
        {
            get { return q_ipodAtom; }
        }

        /// <summary>
        /// Returns a boolean to indicate wither Larger MP4 files is on or off.
        /// </summary>
        public Boolean OptimizeMP4
        {
            get { return q_optimizeMp4; }
        }

        /// <summary>
        /// Returns a string with the video Framerate
        /// </summary>
        public string VideoFramerate
        {
            get { return q_videoFramerate; }
        }

        /// <summary>
        /// Returns a string with the average video bitrate
        /// </summary>
        public string AverageVideoBitrate
        {
            get { return q_avgBitrate; }
        }

        /// <summary>
        /// Returns a string with the video target size
        /// </summary>
        public string VideoTargetSize
        {
            get { return q_videoTargetSize; }
        }

        /// <summary>
        /// Returns a int with the video quality value
        /// </summary>
        public float VideoQuality
        {
            get { return q_videoQuality; }
        }

        #endregion

        #region Audio Settings

        private string q_audioBitrate1;
        private string q_audioBitrate2;
        private string q_audioBitrate3;
        private string q_audioBitrate4;
        private string q_audioEncoder1;
        private string q_audioEncoder2;
        private string q_audioEncoder3;
        private string q_audioEncoder4;
        private string q_audioSamplerate1;
        private string q_audioSamplerate2;
        private string q_audioSamplerate3;
        private string q_audioSamplerate4;
        private string q_audioTrack1;
        private string q_audioTrack2;
        private string q_audioTrack3;
        private string q_audioTrack4;
        private string q_audioTrackMix1;
        private string q_audioTrackMix2;
        private string q_audioTrackMix3;
        private string q_audioTrackMix4;
        private double q_drc1;
        private double q_drc2;
        private double q_drc3;
        private double q_drc4;
        private Boolean q_forcedSubs;
        private string q_subtitles;

        /// <summary>
        /// Returns a string with the selected Audio track
        /// </summary>
        public string AudioTrack1
        {
            get { return q_audioTrack1; }
        }

        /// <summary>
        /// Returns a string with the selected Audio track
        /// </summary>
        public string AudioTrack2
        {
            get { return q_audioTrack2; }
        }

        /// <summary>
        /// Returns a string with the selected Audio track
        /// </summary>
        public string AudioTrack3
        {
            get { return q_audioTrack3; }
        }

        /// <summary>
        /// Returns a string with the selected Audio track
        /// </summary>
        public string AudioTrack4
        {
            get { return q_audioTrack4; }
        }

        /// <summary>
        /// Returns a string with the First selected Audio track Mix
        /// </summary>
        public string AudioTrackMix1
        {
            get { return q_audioTrackMix1; }
        }

        /// <summary>
        /// Returns a string with the First selected Audio track Mix
        /// </summary>
        public string AudioTrackMix2
        {
            get { return q_audioTrackMix2; }
        }

        /// <summary>
        /// Returns a string with the First selected Audio track Mix
        /// </summary>
        public string AudioTrackMix3
        {
            get { return q_audioTrackMix3; }
        }

        /// <summary>
        /// Returns a string with the First selected Audio track Mix
        /// </summary>
        public string AudioTrackMix4
        {
            get { return q_audioTrackMix4; }
        }

        /// <summary>
        /// Returns an String
        /// The Audio Encoder used.
        /// </summary>
        public string AudioEncoder1
        {
            get { return q_audioEncoder1; }
        }

        /// <summary>
        /// Returns an String
        /// The Audio Encoder used.
        /// </summary>
        public string AudioEncoder2
        {
            get { return q_audioEncoder2; }
        }

        /// <summary>
        /// Returns an String
        /// The Audio Encoder used.
        /// </summary>
        public string AudioEncoder3
        {
            get { return q_audioEncoder3; }
        }

        /// <summary>
        /// Returns an String
        /// The Audio Encoder used.
        /// </summary>
        public string AudioEncoder4
        {
            get { return q_audioEncoder4; }
        }

        /// <summary>
        /// Returns a string with the audio bitrate
        /// </summary>
        public string AudioBitrate1
        {
            get { return q_audioBitrate1; }
        }

        /// <summary>
        /// Returns a string with the audio bitrate
        /// </summary>
        public string AudioBitrate2
        {
            get { return q_audioBitrate2; }
        }

        /// <summary>
        /// Returns a string with the audio bitrate
        /// </summary>
        public string AudioBitrate3
        {
            get { return q_audioBitrate3; }
        }

        /// <summary>
        /// Returns a string with the audio bitrate
        /// </summary>
        public string AudioBitrate4
        {
            get { return q_audioBitrate4; }
        }

        /// <summary>
        /// Returns a string with the audio sample rate
        /// </summary>
        public string AudioSamplerate1
        {
            get { return q_audioSamplerate1; }
        }

        /// <summary>
        /// Returns a string with the audio sample rate
        /// </summary>
        public string AudioSamplerate2
        {
            get { return q_audioSamplerate2; }
        }

        /// <summary>
        /// Returns a string with the audio sample rate
        /// </summary>
        public string AudioSamplerate3
        {
            get { return q_audioSamplerate3; }
        }

        /// <summary>
        /// Returns a string with the audio sample rate
        /// </summary>
        public string AudioSamplerate4
        {
            get { return q_audioSamplerate4; }
        }

        /// <summary>
        /// Returns a string with the selected subtitle track
        /// </summary>
        public double DRC1
        {
            get { return q_drc1; }
        }

        /// <summary>
        /// Returns a string with the selected subtitle track
        /// </summary>
        public double DRC2
        {
            get { return q_drc2; }
        }

        /// <summary>
        /// Returns a string with the selected subtitle track
        /// </summary>
        public double DRC3
        {
            get { return q_drc3; }
        }

        /// <summary>
        /// Returns a string with the selected subtitle track
        /// </summary>
        public double DRC4
        {
            get { return q_drc4; }
        }

        /// <summary>
        /// Returns a string with the selected subtitle track
        /// </summary>
        public string Subtitles
        {
            get { return q_subtitles; }
        }

        /// <summary>
        /// Returns a string with the selected subtitle track
        /// </summary>
        public Boolean ForcedSubtitles
        {
            get { return q_forcedSubs; }
        }

        #endregion

        #region Other

        private string q_h264;
        private Boolean q_verbose;

        /// <summary>
        /// Returns a string with the Advanced H264 query string
        /// </summary>
        public string H264Query
        {
            get { return q_h264; }
        }

        /// <summary>
        /// Returns a string with the Advanced H264 query string
        /// </summary>
        public Boolean Verbose
        {
            get { return q_verbose; }
        }

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
            Match videoFramerate = Regex.Match(input, @"-r ([0-9]*)");
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

            Match audioTrack1Mix = Regex.Match(input, @"-6 ([0-9a-z0-9]*)");
            Match audioTrack2Mix = Regex.Match(input, @"-6 ([0-9a-z0-9]*),([0-9a-z0-9]*)");
            Match audioTrack3Mix = Regex.Match(input, @"-6 ([0-9a-z0-9]*),([0-9a-z0-9]*),([0-9a-z0-9]*)");
            Match audioTrack4Mix = Regex.Match(input, @"-6 ([0-9a-z0-9]*),([0-9a-z0-9]*),([0-9a-z0-9]*),([0-9a-z0-9]*)");

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
                    thisQuery.q_dvdTitle = int.Parse(title.ToString().Replace("-t ", ""));

                if (chapters.Success)
                {
                    string[] actTitles = chapters.ToString().Replace("-c ", "").Split('-');
                    thisQuery.q_chaptersStart = int.Parse(actTitles[0]);
                    if (actTitles.Length > 1)
                    {
                        thisQuery.q_chaptersFinish = int.Parse(actTitles[1]);
                    }

                    if ((thisQuery.q_chaptersStart == 1) && (thisQuery.q_chaptersFinish == 0))
                        thisQuery.q_chaptersFinish = thisQuery.q_chaptersStart;
                }
                #endregion

                #region Output Settings

                if (format.Success)
                    thisQuery.q_format = format.ToString().Replace("-f ", "");
                thisQuery.q_largeMp4 = largerMp4.Success;
                thisQuery.q_ipodAtom = ipodAtom.Success;
                thisQuery.q_optimizeMp4 = optimizeMP4.Success;

                #endregion

                #region Picture Tab

                if (width.Success)
                    thisQuery.q_videoWidth = int.Parse(width.ToString().Replace("-w ", ""));

                if (height.Success)
                    thisQuery.q_videoHeight = int.Parse(height.ToString().Replace("-l ", ""));

                if (maxWidth.Success)
                    thisQuery.q_maxWidth = int.Parse(maxWidth.ToString().Replace("-X ", ""));

                if (maxHeight.Success)
                    thisQuery.q_maxHeight = int.Parse(maxHeight.ToString().Replace("-Y ", ""));

                if (crop.Success)
                {
                    thisQuery.q_cropValues = crop.ToString().Replace("--crop ", "");
                    string[] actCropValues = thisQuery.q_cropValues.Split(':');
                    thisQuery.q_croptop = actCropValues[0];
                    thisQuery.q_cropbottom = actCropValues[1];
                    thisQuery.q_cropLeft = actCropValues[2];
                    thisQuery.q_cropRight = actCropValues[3];
                }
                
                thisQuery.q_anamorphic = anamorphic.Success;
                thisQuery.q_looseAnamorphic = lanamorphic.Success;

                #endregion

                #region Filters

                thisQuery.q_decomb = "Off";
                if (decomb.Success)
                {
                    thisQuery.q_decomb = "Default";
                    if (decombValue.Success)
                        thisQuery.q_decomb = decombValue.ToString().Replace("--decomb=", "").Replace("\"", "");
                }

                thisQuery.q_deinterlace = "None";
                if (deinterlace.Success)
                {
                    thisQuery.q_deinterlace = deinterlace.ToString().Replace("--deinterlace=", "").Replace("\"", "");
                    thisQuery.q_deinterlace = thisQuery.q_deinterlace.Replace("fast", "Fast").Replace("slow", "Slow").Replace("slower", "Slower");
                    thisQuery.q_deinterlace = thisQuery.q_deinterlace.Replace("slowest", "Slowest");
                }

                thisQuery.q_denoise = "None";
                if (denoise.Success)
                {
                    thisQuery.q_denoise = denoise.ToString().Replace("--denoise=", "").Replace("\"", "");
                    thisQuery.q_denoise = thisQuery.q_denoise.Replace("weak", "Weak").Replace("medium", "Medium").Replace("strong", "Strong");
                }

                string deblockValue = "";
                thisQuery.q_deBlock = 0;
                if (deblock.Success)
                    deblockValue = deblock.ToString().Replace("--deblock=", "");
                if (deblockValue != "")
                    int.TryParse(deblockValue, out thisQuery.q_deBlock);

                thisQuery.q_detelecine = "Off";
                if (detelecine.Success)
                {
                    thisQuery.q_detelecine = "Default";
                    if (detelecineValue.Success)
                        thisQuery.q_detelecine = detelecineValue.ToString().Replace("--detelecine=", "").Replace("\"", "");
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
                thisQuery.q_videoEncoder = videoEncoderConvertion;
                thisQuery.q_videoFramerate = videoFramerate.Success ? videoFramerate.ToString().Replace("-r ", "") : "Same as source";
                thisQuery.q_grayscale = grayscale.Success;
                thisQuery.q_twoPass = twoPass.Success;
                thisQuery.q_turboFirst = turboFirstPass.Success;
                
                if (videoBitrate.Success)
                    thisQuery.q_avgBitrate = videoBitrate.ToString().Replace("-b ", "");
                if (videoFilesize.Success)
                    thisQuery.q_videoTargetSize = videoFilesize.ToString().Replace("-S ", "");

                if (videoQuality.Success)
                {
                   float qConvert = float.Parse(videoQuality.ToString().Replace("-q ", ""), Culture);
                    //qConvert = Math.Ceiling(qConvert);
                    thisQuery.q_videoQuality = qConvert;
                }
                #endregion

                #region Audio Tab

                // Tracks
                if (noAudio.Success)
                    thisQuery.q_audioTrack1 = "None";
                else if (audioTrack1.Success)
                    thisQuery.q_audioTrack1 = "Automatic";

                if (audioTrack2.Success)
                {
                    string[] audioChan = audioTrack2.ToString().Split(',');
                    thisQuery.q_audioTrack2 = audioChan[1];
                }
                else
                    thisQuery.q_audioTrack2 = "None";

                if (audioTrack3.Success)
                {
                    string[] audioChan = audioTrack3.ToString().Split(',');
                    thisQuery.q_audioTrack3 = audioChan[2];
                }
                else
                    thisQuery.q_audioTrack3 = "None";

                if (audioTrack4.Success)
                {
                    string[] audioChan = audioTrack4.ToString().Split(',');
                    thisQuery.q_audioTrack4 = audioChan[3];
                }
                else
                    thisQuery.q_audioTrack4 = "None";


                // Mixdowns
                thisQuery.q_audioTrackMix1 = "Automatic";
                if (audioTrack1Mix.Success)
                    thisQuery.q_audioTrackMix1 =
                        getMixDown(audioTrack1Mix.ToString().Replace("-6 ", "").Replace(" ", ""));

                thisQuery.q_audioTrackMix2 = "Automatic";
                if (audioTrack2Mix.Success)
                {
                    string[] audio2mix = audioTrack2Mix.ToString().Split(',');
                    thisQuery.q_audioTrackMix2 = getMixDown(audio2mix[1].Trim());
                }

                thisQuery.q_audioTrackMix3 = "Automatic";
                if (audioTrack3Mix.Success)
                {
                    string[] audio3mix = audioTrack3Mix.ToString().Split(',');
                    thisQuery.q_audioTrackMix3 = getMixDown(audio3mix[2].Trim());
                }

                thisQuery.q_audioTrackMix4 = "Automatic";
                if (audioTrack4Mix.Success)
                {
                    string[] audio4mix = audioTrack4Mix.ToString().Split(',');
                    thisQuery.q_audioTrackMix4 = getMixDown(audio4mix[3].Trim());
                }


                // Audio Encoders
                if (audioEncoder1.Success)
                    thisQuery.q_audioEncoder1 = getAudioEncoder(audioEncoder1.ToString().Replace("-E ", ""));

                if (audioEncoder2.Success)
                {
                    string[] audio2enc = audioEncoder2.ToString().Split(',');
                    thisQuery.q_audioEncoder2 = getAudioEncoder(audio2enc[1].Trim());
                }

                if (audioEncoder3.Success)
                {
                    string[] audio3enc = audioEncoder3.ToString().Split(',');
                    thisQuery.q_audioEncoder3 = getAudioEncoder(audio3enc[2].Trim());
                }

                if (audioEncoder4.Success)
                {
                    string[] audio4enc = audioEncoder4.ToString().Split(',');
                    thisQuery.q_audioEncoder4 = getAudioEncoder(audio4enc[3].Trim());
                }


                // Audio Bitrate
                thisQuery.q_audioBitrate1 = "";
                if (audioBitrate1.Success)
                {
                    thisQuery.q_audioBitrate1 = audioBitrate1.ToString().Replace("-B ", "").Trim();
                    if (audioBitrate1.ToString().Replace("-B ", "").Trim() == "0") thisQuery.q_audioBitrate1 = "Auto";
                }

                thisQuery.q_audioBitrate2 = "";
                if (audioBitrate2.Success && audioTrack2.Success)
                {
                    string[] audioBitrateSelect = audioBitrate2.ToString().Split(',');
                    if (audioBitrateSelect[1].Trim() == "0") audioBitrateSelect[1] = "Auto";
                    thisQuery.q_audioBitrate2 = audioBitrateSelect[1].Trim();
                }

                thisQuery.q_audioBitrate3 = "";
                if (audioBitrate3.Success && audioTrack3.Success)
                {
                    string[] audioBitrateSelect = audioBitrate3.ToString().Split(',');
                    if (audioBitrateSelect[2].Trim() == "0") audioBitrateSelect[2] = "Auto";
                    thisQuery.q_audioBitrate3 = audioBitrateSelect[2].Trim();
                }

                thisQuery.q_audioBitrate4 = "";
                if (audioBitrate4.Success)
                {
                    string[] audioBitrateSelect = audioBitrate4.ToString().Split(',');
                    if (audioBitrateSelect[3].Trim() == "0") audioBitrateSelect[3] = "Auto";
                    thisQuery.q_audioBitrate4 = audioBitrateSelect[3].Trim();
                }


                // Audio Sample Rate
                // Make sure to change 0 to Auto
                thisQuery.q_audioSamplerate1 = "Auto";
                if (audioSampleRate1.Success)
                {
                    thisQuery.q_audioSamplerate1 = audioSampleRate1.ToString().Replace("-R ", "").Trim();
                    if (thisQuery.q_audioSamplerate1 == "0") thisQuery.q_audioSamplerate1 = "Auto";
                }


                if (audioSampleRate2.Success)
                {
                    string[] audioSRSelect = audioSampleRate2.ToString().Split(',');
                    if (audioSRSelect[1] == "0") audioSRSelect[1] = "Auto";
                    thisQuery.q_audioSamplerate2 = audioSRSelect[1].Trim();
                }

                if (audioSampleRate3.Success)
                {
                    string[] audioSRSelect = audioSampleRate3.ToString().Split(',');
                    if (audioSRSelect[2] == "0") audioSRSelect[2] = "Auto";
                    thisQuery.q_audioSamplerate3 = audioSRSelect[2].Trim();
                }

                if (audioSampleRate4.Success)
                {
                    string[] audioSRSelect = audioSampleRate4.ToString().Split(',');
                    if (audioSRSelect[3] == "0") audioSRSelect[3] = "Auto";
                    thisQuery.q_audioSamplerate4 = audioSRSelect[3].Trim();
                }

                // DRC
                float drcValue;

                thisQuery.q_drc1 = 1;
                if (drc1.Success)
                {
                    string value = drc1.ToString().Replace("-D ", "");
                    float.TryParse(value, out drcValue);
                    thisQuery.q_drc1 = drcValue;
                }

                thisQuery.q_drc2 = 1;
                if (drc2.Success)
                {
                    string[] drcPoint = drc2.ToString().Split(',');
                    float.TryParse(drcPoint[1], out drcValue);
                    thisQuery.q_drc2 = drcValue;
                }

                thisQuery.q_drc3 = 1;
                if (drc3.Success)
                {
                    string[] drcPoint = drc3.ToString().Split(',');
                    float.TryParse(drcPoint[2], out drcValue);
                    thisQuery.q_drc3 = drcValue;
                }

                thisQuery.q_drc4 = 1;
                if (drc4.Success)
                {
                    string[] drcPoint = drc4.ToString().Split(',');
                    float.TryParse(drcPoint[3], out drcValue);
                    thisQuery.q_drc4 = drcValue;
                }

                // Subtitle Stuff
                if (subtitles.Success)
                    thisQuery.q_subtitles = subtitles.ToString().Replace("-s ", "");
                else
                    thisQuery.q_subtitles = subScan.Success ? "Autoselect" : "None";

                thisQuery.q_forcedSubs = forcedSubtitles.Success;

                #endregion

                #region Chapters Tab
                if (chapterMarkersFileMode.Success || chapterMarkers.Success)
                    thisQuery.q_chapterMarkers = true;
                #endregion

                #region H.264 and other

                //
                //H264 Tab
                //
                if (x264.Success)
                    thisQuery.q_h264 = x264.ToString().Replace("-x ", "");

                //
                //Progam Options
                //
                thisQuery.q_verbose = verbose.Success;

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
            switch (mixdown)
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