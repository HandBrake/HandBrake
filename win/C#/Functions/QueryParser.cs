/*  QueryParser.cs $
 	
 	   This file is part of the HandBrake source code.
 	   Homepage: <http://handbrake.fr>.
 	   It may be used under the terms of the GNU General Public License. */

using System;
using System.Collections.Generic;
using System.Text;
using System.Text.RegularExpressions;
using System.Windows.Forms;

namespace Handbrake.Functions
{
    class QueryParser
    {

        // All the Main Window GUI options
        #region Varibles

        #region Source
        private string q_source;
        /// <summary>
        /// Returns a String 
        /// Full path of the source.
        /// </summary>
        public string Source
        {
            get
            {
                return this.q_source;
            }
        }
        private int q_dvdTitle;
        /// <summary>
        /// Returns an Integer
        /// DVD Title number.
        /// </summary>
        public int DVDTitle
        {
            get
            {
                return this.q_dvdTitle;
            }
        }

        private int q_chaptersStart;
        /// <summary>
        /// Returns an Int
        /// DVD Chapter number or chapter range.
        /// </summary>
        public int DVDChapterStart
        {
            get
            {
                return this.q_chaptersStart;
            }
        }

        private int q_chaptersFinish;
        /// <summary>
        /// Returns an Int
        /// DVD Chapter number or chapter range.
        /// </summary>
        public int DVDChapterFinish
        {
            get
            {
                return this.q_chaptersFinish;
            }
        }
#endregion

        #region Destination

        private string q_destination;
        /// <summary>
        /// Returns a String 
        /// Full path of the destination.
        /// </summary>
        public string Destination
        {
            get
            {
                return this.q_destination;
            }
        }

        private string q_format;
        /// <summary>
        /// Returns a String 
        /// Full path of the destination.
        /// </summary>
        public string Format
        {
            get
            {
                return this.q_format;
            }
        }

        private string q_videoEncoder;
        /// <summary>
        /// Returns an String
        /// The Video Encoder used.
        /// </summary>
        public string VideoEncoder
        {
            get
            {
                return this.q_videoEncoder;
            }
        }
        #endregion

        #region Picture Settings
        private int q_videoWidth;
        /// <summary>
        /// Returns an Int
        /// The selected Width for the encoding.
        /// </summary>
        public int Width
        {
            get
            {
                return this.q_videoWidth;
            }
        }

        private int q_videoHeight;
        /// <summary>
        /// Returns an Int
        /// The selected Height for the encoding.
        /// </summary>
        public int Height
        {
            get
            {
                return this.q_videoHeight;
            }
        }

        private string q_cropValues;
        /// <summary>
        /// Returns an String
        /// Cropping values.
        /// </summary>
        public string CropValues
        {
            get
            {
                return this.q_cropValues;
            }
        }

        private string q_croptop;
        /// <summary>
        /// Returns an String
        /// Cropping values.
        /// </summary>
        public string CropTop
        {
            get
            {
                return this.q_croptop;
            }
        }

        private string q_cropbottom;
        /// <summary>
        /// Returns an String
        /// Cropping values.
        /// </summary>
        public string CropBottom
        {
            get
            {
                return this.q_cropbottom;
            }
        }

        private string q_cropLeft;
        /// <summary>
        /// Returns an String
        /// Cropping values.
        /// </summary>
        public string CropLeft
        {
            get
            {
                return this.q_cropLeft;
            }
        }

        private string q_cropRight;
        /// <summary>
        /// Returns an String
        /// Cropping values.
        /// </summary>
        public string CropRight
        {
            get
            {
                return this.q_cropRight;
            }
        }

        private Boolean q_detelecine;
        /// <summary>
        /// Returns a boolean to indicate wither DeTelecine is on or off
        /// </summary>
        public Boolean DeTelecine
        {
            get
            {
                return this.q_detelecine;
            }
        }

        private Boolean q_vfr;
        /// <summary>
        /// Returns a boolean to indicate wither DeTelecine is on or off
        /// </summary>
        public Boolean VFR
        {
            get
            {
                return this.q_vfr;
            }
        }

        private Boolean q_deBlock;
        /// <summary>
        /// Returns a boolean to indicate wither DeBlock is on or off.
        /// </summary>
        public Boolean DeBlock
        {
            get
            {
                return this.q_deBlock;
            }
        }

        private string q_deinterlace;
        /// <summary>
        /// Returns a string with the De-Interlace option used.
        /// </summary>
        public string DeInterlace
        {
            get
            {
                return this.q_deinterlace;
            }
        }

        private string q_denoise;
        /// <summary>
        /// Returns a string with the DeNoise option used.
        /// </summary>
        public string DeNoise
        {
            get
            {
                return this.q_denoise;
            }
        }

        private Boolean q_anamorphic;
        /// <summary>
        /// Returns a boolean to indicate wither Anamorphic is on or off.
        /// </summary>
        public Boolean Anamorphic
        {
            get
            {
                return this.q_anamorphic;
            }
        }

        private Boolean q_looseAnamorphic;
        /// <summary>
        /// Returns a boolean to indicate wither Anamorphic is on or off.
        /// </summary>
        public Boolean LooseAnamorphic
        {
            get
            {
                return this.q_looseAnamorphic;
            }
        }

        private Boolean q_chapterMarkers;
        /// <summary>
        /// Returns a boolean to indicate wither Chapter Markers is on or off.
        /// </summary>
        public Boolean ChapterMarkers
        {
            get
            {
                return this.q_chapterMarkers;
            }
        }
        #endregion

        #region Video Settings
        private Boolean q_grayscale;
        /// <summary>
        /// Returns a boolean to indicate wither Grayscale is on or off.
        /// </summary>
        public Boolean Grayscale
        {
            get
            {
                return this.q_grayscale;
            }
        }

        private Boolean q_twoPass;
        /// <summary>
        /// Returns a boolean to indicate wither Two Pass Encoding is on or off.
        /// </summary>
        public Boolean TwoPass
        {
            get
            {
                return this.q_twoPass;
            }
        }

        private Boolean q_turboFirst;
        /// <summary>
        /// Returns a boolean to indicate wither Chapter Markers is on or off.
        /// </summary>
        public Boolean TurboFirstPass
        {
            get
            {
                return this.q_turboFirst;
            }
        }

        private Boolean q_largeMp4;
        /// <summary>
        /// Returns a boolean to indicate wither Larger MP4 files is on or off.
        /// </summary>
        public Boolean LargeMP4
        {
            get
            {
                return this.q_largeMp4;
            }
        }

        private Boolean q_ipodAtom;
        /// <summary>
        /// Returns a boolean to indicate wither Larger MP4 files is on or off.
        /// </summary>
        public Boolean IpodAtom
        {
            get
            {
                return this.q_ipodAtom;
            }
        }

        private Boolean q_optimizeMp4;
        /// <summary>
        /// Returns a boolean to indicate wither Larger MP4 files is on or off.
        /// </summary>
        public Boolean OptimizeMP4
        {
            get
            {
                return this.q_optimizeMp4;
            }
        }

        private string q_videoFramerate;
        /// <summary>
        /// Returns a string with the video Framerate
        /// </summary>
        public string VideoFramerate
        {
            get
            {
                return this.q_videoFramerate;
            }
        }

        private string q_avgBitrate;
        /// <summary>
        /// Returns a string with the average video bitrate
        /// </summary>
        public string AverageVideoBitrate
        {
            get
            {
                return this.q_avgBitrate;
            }
        }

        private string q_videoTargetSize;
        /// <summary>
        /// Returns a string with the video target size
        /// </summary>
        public string VideoTargetSize
        {
            get
            {
                return this.q_videoTargetSize;
            }
        }

        private int q_videoQuality;
        /// <summary>
        /// Returns a int with the video quality value
        /// </summary>
        public int VideoQuality
        {
            get
            {
                return this.q_videoQuality;
            }
        }

        #endregion

        #region Audio Settings
        private string q_audioTrack1;
        /// <summary>
        /// Returns a string with the First selected Audio track
        /// </summary>
        public string AudioTrack1
        {
            get
            {
                return this.q_audioTrack1;
            }
        }

        private string q_audioTrack2;
        /// <summary>
        /// Returns a string with the First selected Audio track
        /// </summary>
        public string AudioTrack2
        {
            get
            {
                return this.q_audioTrack2;
            }
        }

        private string q_audioTrackMix1;
        /// <summary>
        /// Returns a string with the First selected Audio track Mix
        /// </summary>
        public string AudioTrackMix1
        {
            get
            {
                return this.q_audioTrackMix1;
            }
        }

        private string q_audioTrackMix2;
        /// <summary>
        /// Returns a string with the First selected Audio track Mix
        /// </summary>
        public string AudioTrackMix2
        {
            get
            {
                return this.q_audioTrackMix2;
            }
        }

        private string q_audioEncoder1;
        /// <summary>
        /// Returns an String
        /// The Audio Encoder used.
        /// </summary>
        public string AudioEncoder1
        {
            get
            {
                return this.q_audioEncoder1;
            }
        }

        private string q_audioEncoder2;
        /// <summary>
        /// Returns an String
        /// The Audio Encoder used.
        /// </summary>
        public string AudioEncoder2
        {
            get
            {
                return this.q_audioEncoder2;
            }
        }

        private string q_audioBitrate1;
        /// <summary>
        /// Returns a string with the audio bitrate
        /// </summary>
        public string AudioBitrate1
        {
            get
            {
                return this.q_audioBitrate1;
            }
        }

        private string q_audioBitrate2;
        /// <summary>
        /// Returns a string with the audio bitrate
        /// </summary>
        public string AudioBitrate2
        {
            get
            {
                return this.q_audioBitrate2;
            }
        }

        private string q_audioSamplerate1;
        /// <summary>
        /// Returns a string with the audio sample rate
        /// </summary>
        public string AudioSamplerate1
        {
            get
            {
                return this.q_audioSamplerate1;
            }
        }

        private string q_audioSamplerate2;
        /// <summary>
        /// Returns a string with the audio sample rate
        /// </summary>
        public string AudioSamplerate2
        {
            get
            {
                return this.q_audioSamplerate2;
            }
        }

        private double q_drc;
        /// <summary>
        /// Returns a string with the selected subtitle track
        /// </summary>
        public double DRC
        {
            get
            {
                return this.q_drc;
            }
        }

        private string q_subtitles;
        /// <summary>
        /// Returns a string with the selected subtitle track
        /// </summary>
        public string Subtitles
        {
            get
            {
                return this.q_subtitles;
            }
        }

        private Boolean q_forcedSubs;
        /// <summary>
        /// Returns a string with the selected subtitle track
        /// </summary>
        public Boolean ForcedSubtitles
        {
            get
            {
                return this.q_forcedSubs;
            }
        }

        #endregion

        #region Other
        private string q_h264;
        /// <summary>
        /// Returns a string with the Advanced H264 query string
        /// </summary>
        public string H264Query
        {
            get
            {
                return this.q_h264;
            }
        }
        private Boolean q_verbose;
        /// <summary>
        /// Returns a string with the Advanced H264 query string
        /// </summary>
        public Boolean Verbose
        {
            get
            {
                return this.q_verbose;
            }
        }
        #endregion

        #endregion

        // Takes in a query which can be in any order and parses it. All varibles are then set so they can be used elsewhere.
        public static QueryParser Parse(String input)
        {
            QueryParser thisQuery = new QueryParser();

            #region Regular Expressions
            //Source
            Regex r1 = new Regex(@"(-i)(?:\s\"")([a-zA-Z0-9:\\\s\.]+)(?:\"")");
            Match source = r1.Match(input.Replace('"', '\"'));
            Match title = Regex.Match(input, @"-t ([0-9]*)");
            Match chapters = Regex.Match(input, @"-c ([0-9-]*)");
            Match format = Regex.Match(input, @"-f ([a-z0-9a-z0-9a-z0-9]*)");

            //Destination
            Regex r2 = new Regex(@"(-o)(?:\s\"")([a-zA-Z0-9_\-:\\\s\.]+)(?:\"")");
            Match destination = r2.Match(input.Replace('"', '\"'));
            Match videoEncoder = Regex.Match(input, @"-e ([a-zA-Z0-9]*)");

            //Picture Settings Tab
            Match width = Regex.Match(input, @"-w ([0-9]*)");
            Match height = Regex.Match(input, @"-l ([0-9]*)");
            Match deinterlace = Regex.Match(input, @"--deinterlace=\""([a-zA-Z]*)\""");
            Match denoise = Regex.Match(input, @"--denoise=\""([a-zA-Z]*)\""");
            Match deblock = Regex.Match(input, @"--deblock");
            Match detelecine = Regex.Match(input, @"--detelecine");
            Match anamorphic = Regex.Match(input, @"-p ");
            Match chapterMarkers = Regex.Match(input, @"-m");
            Match crop = Regex.Match(input, @"--crop ([0-9]):([0-9]):([0-9]):([0-9])");
            Match vfr = Regex.Match(input, @"-V");
            Match lanamorphic = Regex.Match(input, @"-P");

            //Video Settings Tab
            Match videoFramerate = Regex.Match(input, @"-r ([0-9]*)");
            Match videoBitrate = Regex.Match(input, @"-b ([0-9]*)");
            Match videoQuality = Regex.Match(input, @"-q ([0-9.]*)");
            Match videoFilesize = Regex.Match(input, @"-S ([0-9.]*)");
            Match twoPass = Regex.Match(input, @"-2");
            Match turboFirstPass = Regex.Match(input, @"-T");
            Match grayscale = Regex.Match(input, @"-g");
            Match largerMp4 = Regex.Match(input, @"-4");
            Match ipodAtom = Regex.Match(input, @"-I");
            Match optimizeMP4 = Regex.Match(input, @"-O");

            //Audio Settings Tab
            Match audioTrack1 = Regex.Match(input, @"-a ([0-9]*)");
            Match audioTrack2 = Regex.Match(input, @"-a ([0-9]*),([0-9]*)");
            Match audioTrack1Mix = Regex.Match(input, @"-6 ([0-9a-z0-9]*)");  
            Match audioTrack2Mix = Regex.Match(input, @"-6 ([0-9a-z0-9]*),([0-9a-z0-9]*)");  
            Match audioEncoder1 = Regex.Match(input, @"-E ([a-zA-Z0-9+]*)");
            Match audioEncoder2 = Regex.Match(input, @"-E ([a-zA-Z0-9+]*),([a-zA-Z0-9+]*)");
            Match audioBitrate1 = Regex.Match(input, @"-B ([0-9]*)");
            Match audioBitrate2 = Regex.Match(input, @"-B ([0-9]*),([0-9]*)");
            Match audioSampleRate1 = Regex.Match(input, @"-R ([0-9.]*)");
            Match audioSampleRate2 = Regex.Match(input, @"-R ([0-9.]*),([0-9.]*)");

            Match drc = Regex.Match(input, @"-D ([0-9.]*)");
            Match subtitles = Regex.Match(input, @"-s ([0-9a-zA-Z]*)");
            Match subScan = Regex.Match(input, @"-U");
            Match forcedSubtitles = Regex.Match(input, @"-F");

            //H264 Tab
            Match x264 = Regex.Match(input, @"-x ([,a-zA-Z0-9=:-]*)");

            //Program Options
            Match verbose = Regex.Match(input, @"-v");
            #endregion

            #region Set Varibles
            try
            {
                /*
                 * Source
                 */
                #region Source Tab

                thisQuery.q_source = source.ToString().Replace("-i ", "").Replace("\"", "");
                if (title.Success != false)
                    thisQuery.q_dvdTitle = int.Parse(title.ToString().Replace("-t ", ""));

                if (chapters.Success != false)
                {
                    string[] actTitles = new string[2];
                    actTitles = chapters.ToString().Replace("-c ", "").Split('-');
                    thisQuery.q_chaptersStart = int.Parse(actTitles[0]);
                    if (actTitles.Length > 1)
                    {
                        thisQuery.q_chaptersFinish = int.Parse(actTitles[1]);
                    }

                    if ((thisQuery.q_chaptersStart == 1) && (thisQuery.q_chaptersFinish == 0))
                        thisQuery.q_chaptersFinish = thisQuery.q_chaptersStart;
                }

                if (format.Success != false)
                    thisQuery.q_format = format.ToString().Replace("-f ", "");

                #endregion

                /*
                 * Destination
                 */
                #region Destination
                thisQuery.q_destination = destination.ToString().Replace("-o ", "").Replace("\"", "");

                string videoEncoderConvertion;
                string audioEncoderConvertion;

                videoEncoderConvertion = videoEncoder.ToString().Replace("-e ", "");
                switch (videoEncoderConvertion)
                {
                    case "ffmpeg":
                        videoEncoderConvertion = "Mpeg 4";
                        break;
                    case "xvid":
                        videoEncoderConvertion = "Xvid";
                        break;
                    case "x264":
                        videoEncoderConvertion = "H.264";
                        break;
                    case "x264b13":
                        videoEncoderConvertion = "H.264 Baseline 1.3";
                        break;
                    case "x264b30":
                        videoEncoderConvertion = "H.264 (iPod)";
                        break;
                    default:
                        videoEncoderConvertion = "Mpeg 4";
                        break;
                }
                thisQuery.q_videoEncoder = videoEncoderConvertion;

                #endregion

                /*
                 * Picture Settings Tab
                 */
                #region Picture Tab

                if (width.Success != false)
                    thisQuery.q_videoWidth = int.Parse(width.ToString().Replace("-w ", ""));

                if (height.Success != false)
                    thisQuery.q_videoHeight = int.Parse(height.ToString().Replace("-l ", ""));


                if (crop.Success != false)
                {
                    thisQuery.q_cropValues = crop.ToString().Replace("--crop ", "");
                    string[] actCropValues = new string[3];
                    actCropValues = thisQuery.q_cropValues.Split(':');
                    thisQuery.q_croptop = actCropValues[0];
                    thisQuery.q_cropbottom = actCropValues[1];
                    thisQuery.q_cropLeft = actCropValues[2];
                    thisQuery.q_cropRight = actCropValues[3];
                }

                thisQuery.q_detelecine = detelecine.Success;
                thisQuery.q_deBlock = deblock.Success;

                thisQuery.q_deinterlace = "None";
                if (deinterlace.Success != false)
                {
                    switch (deinterlace.ToString().Replace("--deinterlace=", "").Replace("\"",""))
                    {
                        case "fast":
                            thisQuery.q_deinterlace = "Fast";
                            break;
                        case "slow":
                            thisQuery.q_deinterlace = "Slow";
                            break;
                        case "slower":
                            thisQuery.q_deinterlace = "Slower";
                            break;
                        case "slowest":
                            thisQuery.q_deinterlace = "Slowest";
                            break;
                        default:
                            thisQuery.q_deinterlace = "None";
                            break;
                    }
                }

                thisQuery.q_denoise = "None";
                if (denoise.Success != false)
                {
                    switch (denoise.ToString().Replace("--denoise=", ""))
                    {
                        case "weak":
                            thisQuery.q_denoise = "Weak";
                            break;
                        case "medium":
                            thisQuery.q_denoise = "Medium";
                            break;
                        case "strong":
                            thisQuery.q_denoise = "Strong";
                            break;
                        default:
                            thisQuery.q_denoise = "None";
                            break;
                    }

                }
                thisQuery.q_anamorphic = anamorphic.Success;
                thisQuery.q_chapterMarkers = chapterMarkers.Success;
                thisQuery.q_vfr = vfr.Success;
                thisQuery.q_looseAnamorphic = lanamorphic.Success;

                #endregion

                /*
                 * Video Settings Tab
                 */
                #region Video
                thisQuery.q_grayscale = grayscale.Success;
                thisQuery.q_twoPass = twoPass.Success;
                thisQuery.q_turboFirst = turboFirstPass.Success;
                thisQuery.q_largeMp4 = largerMp4.Success;
                if (videoFramerate.Success != false)
                    thisQuery.q_videoFramerate = videoFramerate.ToString().Replace("-r ", "");
                else
                    thisQuery.q_videoFramerate = "Same as source";

                if (videoBitrate.Success != false)
                    thisQuery.q_avgBitrate = videoBitrate.ToString().Replace("-b ", "");
                if (videoFilesize.Success != false)
                    thisQuery.q_videoTargetSize = videoFilesize.ToString().Replace("-S ", "");

                double qConvert = 0;
                if (videoQuality.Success != false)
                {
                    qConvert = double.Parse(videoQuality.ToString().Replace("-q ", ""), Functions.CLI.Culture) * 100;
                    qConvert = System.Math.Ceiling(qConvert);
                    thisQuery.q_videoQuality = int.Parse(qConvert.ToString());
                }
                thisQuery.q_ipodAtom = ipodAtom.Success;
                thisQuery.q_optimizeMp4 = optimizeMP4.Success;

                #endregion

                /*
                 * Audio Settings Tab
                 */
                #region Audio
                // Tracks
                if (audioTrack1.Success != false)
                    thisQuery.q_audioTrack1 = audioTrack1.ToString().Replace("-a ", "");
                else
                    thisQuery.q_audioTrack1 = "Automatic";

                if (audioTrack2.Success != false)
                {
                    string[] audioChan = audioTrack2.ToString().Split(',');
                    thisQuery.q_audioTrack2 = audioChan[1];
                }
                else
                    thisQuery.q_audioTrack2 = "None";
    
                // Mixdowns
                thisQuery.q_audioTrackMix1 = "Automatic";
                if (audioTrack1Mix.Success != false)
                {
                    switch (audioTrack1Mix.ToString().Replace("-6 ", "").Replace(" ", ""))
                    {
                        case "mono":
                            thisQuery.q_audioTrackMix1 = "Mono";
                            break;
                        case "stereo":
                            thisQuery.q_audioTrackMix1 = "Stereo";
                            break;
                        case "dpl1":
                            thisQuery.q_audioTrackMix1 = "Dolby Surround";
                            break;
                        case "dpl2":
                            thisQuery.q_audioTrackMix1 = "Dolby Pro Logic II";
                            break;
                        case "6ch":
                            thisQuery.q_audioTrackMix1 = "6 Channel Discrete";
                            break;
                        default:
                            thisQuery.q_audioTrackMix1 = "Automatic";
                            break;
                    }
                }

                thisQuery.q_audioTrackMix2 = "Automatic";
                if (audioTrack2Mix.Success != false)
                {
                    string[] audio2mix = audioTrack2Mix.ToString().Split(',');
                    audio2mix[1] = audio2mix[1].Trim();
                    switch (audio2mix[1])
                    {
                        case "mono":
                            thisQuery.q_audioTrackMix2 = "Mono";
                            break;
                        case "stereo":
                            thisQuery.q_audioTrackMix2 = "Stereo";
                            break;
                        case "dpl1":
                            thisQuery.q_audioTrackMix2 = "Dolby Surround";
                            break;
                        case "dpl2":
                            thisQuery.q_audioTrackMix2 = "Dolby Pro Logic II";
                            break;
                        case "6ch":
                            thisQuery.q_audioTrackMix2 = "6 Channel Discrete";
                            break;
                        default:
                            thisQuery.q_audioTrackMix2 = "Automatic";
                            break;
                    }
                }

                // Audio Encoders
                if (audioEncoder1.Success != false)
                {
                    audioEncoderConvertion = audioEncoder1.ToString().Replace("-E ", "");
                    switch (audioEncoderConvertion)
                    {
                        case "faac":
                            audioEncoderConvertion = "AAC";
                            break;
                        case "lame":
                            audioEncoderConvertion = "MP3";
                            break;
                        case "vorbis":
                            audioEncoderConvertion = "Vorbis";
                            break;
                        case "ac3":
                            audioEncoderConvertion = "AC3";
                            break;
                        case "aac+ac3":
                            audioEncoderConvertion = "AAC + AC3";
                            break;
                        default:
                            audioEncoderConvertion = "AAC";
                            break;
                    }
                    thisQuery.q_audioEncoder1 = audioEncoderConvertion;
                }

                if (audioEncoder2.Success != false)
                {
                    audioEncoderConvertion = audioEncoder2.ToString().Replace("-E ", "");
                    string[] audio2enc = audioEncoder2.ToString().Split(',');
                    audio2enc[1] = audio2enc[1].Trim();
                    switch (audio2enc[1])
                    {
                        case "faac":
                            audioEncoderConvertion = "AAC";
                            break;
                        case "lame":
                            audioEncoderConvertion = "MP3";
                            break;
                        case "vorbis":
                            audioEncoderConvertion = "Vorbis";
                            break;
                        case "ac3":
                            audioEncoderConvertion = "AC3";
                            break;
                        case "aac+ac3":
                            audioEncoderConvertion = "AAC + AC3";
                            break;
                        default:
                            audioEncoderConvertion = "AAC";
                            break;
                    }
                    thisQuery.q_audioEncoder2 = audioEncoderConvertion;
                }


                // Audio Bitrate
                if (audioBitrate1.Success != false)
                    thisQuery.q_audioBitrate1 = audioBitrate1.ToString().Replace("-B ", "").Trim();
                else
                    thisQuery.q_audioBitrate1 = "";

                if (audioBitrate2.Success != false)
                {
                    string[] audioBitrateSelect = audioBitrate2.ToString().Split(',');
                    thisQuery.q_audioBitrate2 = audioBitrateSelect[1].Trim();
                }
                else
                    thisQuery.q_audioBitrate2 = "";

                // Audio Sample Rate
                if (audioSampleRate1.Success != false)
                    thisQuery.q_audioSamplerate1 = audioSampleRate1.ToString().Replace("-R ", "").Trim();

                if (audioSampleRate2.Success != false)
                {
                    string[] audioSRSelect = audioSampleRate2.ToString().Split(',');
                    thisQuery.q_audioSamplerate2 = audioSRSelect[1].Trim();
                }


                if (subtitles.Success != false)
                    thisQuery.q_subtitles = subtitles.ToString().Replace("-s ", "");
                else
                {
                    if (subScan.Success)
                        thisQuery.q_subtitles = "Autoselect";
                    else
                        thisQuery.q_subtitles = "None";
                }

                if (drc.Success != false)
                {
                    string value = drc.ToString().Replace("-D ", "");
                    float drcValue = float.Parse(value);
                    drcValue = drcValue * 10;
                    thisQuery.q_drc = drcValue;
                }
                else
                    thisQuery.q_drc = 0;

                thisQuery.q_forcedSubs = forcedSubtitles.Success;

                #endregion

                //
                //H264 tab and other 
                //
                #region h264 and other
                //
                //H264 Tab
                //
                if (x264.Success != false)
                {
                    thisQuery.q_h264 = x264.ToString().Replace("-x ", "");
                }

                //
                //Progam Options
                //
                thisQuery.q_verbose = verbose.Success;
                #endregion
            }
            catch (Exception exc)
            {
                MessageBox.Show("An error has occured in the Query Parser. Please report this error on the forum in the 'Windows' support section. \n\n" + exc.ToString(), "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
            #endregion

            return thisQuery;
        }
    }
}