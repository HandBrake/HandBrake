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
        /// Returns a string with the selected Audio track
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
        /// Returns a string with the selected Audio track
        /// </summary>
        public string AudioTrack2
        {
            get
            {
                return this.q_audioTrack2;
            }
        }

        private string q_audioTrack3;
        /// <summary>
        /// Returns a string with the selected Audio track
        /// </summary>
        public string AudioTrack3
        {
            get
            {
                return this.q_audioTrack3;
            }
        }

        private string q_audioTrack4;
        /// <summary>
        /// Returns a string with the selected Audio track
        /// </summary>
        public string AudioTrack4
        {
            get
            {
                return this.q_audioTrack4;
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

        private string q_audioTrackMix3;
        /// <summary>
        /// Returns a string with the First selected Audio track Mix
        /// </summary>
        public string AudioTrackMix3
        {
            get
            {
                return this.q_audioTrackMix3;
            }
        }

        private string q_audioTrackMix4;
        /// <summary>
        /// Returns a string with the First selected Audio track Mix
        /// </summary>
        public string AudioTrackMix4
        {
            get
            {
                return this.q_audioTrackMix4;
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

        private string q_audioEncoder3;
        /// <summary>
        /// Returns an String
        /// The Audio Encoder used.
        /// </summary>
        public string AudioEncoder3
        {
            get
            {
                return this.q_audioEncoder3;
            }
        }

        private string q_audioEncoder4;
        /// <summary>
        /// Returns an String
        /// The Audio Encoder used.
        /// </summary>
        public string AudioEncoder4
        {
            get
            {
                return this.q_audioEncoder4;
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

        private string q_audioBitrate3;
        /// <summary>
        /// Returns a string with the audio bitrate
        /// </summary>
        public string AudioBitrate3
        {
            get
            {
                return this.q_audioBitrate3;
            }
        }

        private string q_audioBitrate4;
        /// <summary>
        /// Returns a string with the audio bitrate
        /// </summary>
        public string AudioBitrate4
        {
            get
            {
                return this.q_audioBitrate4;
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

        private string q_audioSamplerate3;
        /// <summary>
        /// Returns a string with the audio sample rate
        /// </summary>
        public string AudioSamplerate3
        {
            get
            {
                return this.q_audioSamplerate3;
            }
        }

        private string q_audioSamplerate4;
        /// <summary>
        /// Returns a string with the audio sample rate
        /// </summary>
        public string AudioSamplerate4
        {
            get
            {
                return this.q_audioSamplerate4;
            }
        }

        private double q_drc1;
        /// <summary>
        /// Returns a string with the selected subtitle track
        /// </summary>
        public double DRC1
        {
            get
            {
                return this.q_drc1;
            }
        }

        private double q_drc2;
        /// <summary>
        /// Returns a string with the selected subtitle track
        /// </summary>
        public double DRC2
        {
            get
            {
                return this.q_drc2;
            }
        }

        private double q_drc3;
        /// <summary>
        /// Returns a string with the selected subtitle track
        /// </summary>
        public double DRC3
        {
            get
            {
                return this.q_drc3;
            }
        }

        private double q_drc4;
        /// <summary>
        /// Returns a string with the selected subtitle track
        /// </summary>
        public double DRC4
        {
            get
            {
                return this.q_drc4;
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

        /// <summary>
        /// Takes in a query which can be in any order and parses it. 
        /// All varibles are then set so they can be used elsewhere.
        /// </summary>
        /// <param name="input">A ClI Query</param>
        /// <returns>A Parsed Query</returns>
 
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
            Match anamorphic = Regex.Match(input, @" -p ");
            Match chapterMarkers = Regex.Match(input, @" -m");
            Match crop = Regex.Match(input, @"--crop ([0-9]):([0-9]):([0-9]):([0-9])");
            Match vfr = Regex.Match(input, @" -V");
            Match lanamorphic = Regex.Match(input, @"-P");

            //Video Settings Tab
            Match videoFramerate = Regex.Match(input, @"-r ([0-9]*)");
            Match videoBitrate = Regex.Match(input, @"-b ([0-9]*)");
            Match videoQuality = Regex.Match(input, @"-q ([0-9.]*)");
            Match videoFilesize = Regex.Match(input, @"-S ([0-9.]*)");
            Match twoPass = Regex.Match(input, @" -2");
            Match turboFirstPass = Regex.Match(input, @" -T");
            Match grayscale = Regex.Match(input, @" -g");
            Match largerMp4 = Regex.Match(input, @" -4");
            Match ipodAtom = Regex.Match(input, @" -I");
            Match optimizeMP4 = Regex.Match(input, @" -O");

            //Audio Settings Tab
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

            Match audioBitrate1 = Regex.Match(input, @"-B ([0-9]*)");
            Match audioBitrate2 = Regex.Match(input, @"-B ([0-9]*),([0-9]*)");
            Match audioBitrate3 = Regex.Match(input, @"-B ([0-9]*),([0-9]*),([0-9]*)");
            Match audioBitrate4 = Regex.Match(input, @"-B ([0-9]*),([0-9]*),([0-9]*),([0-9]*)");

            Match audioSampleRate1 = Regex.Match(input, @"-R ([0-9.]*)");
            Match audioSampleRate2 = Regex.Match(input, @"-R ([0-9.]*),([0-9.]*)");
            Match audioSampleRate3 = Regex.Match(input, @"-R ([0-9.]*),([0-9.]*),([0-9.]*)");
            Match audioSampleRate4 = Regex.Match(input, @"-R ([0-9.]*),([0-9.]*),([0-9.]*),([0-9.]*)");

            Match drc1 = Regex.Match(input, @"-D ([0-9.]*)");
            Match drc2 = Regex.Match(input, @"-D ([0-9.]*),([0-9.]*)");
            Match drc3 = Regex.Match(input, @"-D ([0-9.]*),([0-9.]*),([0-9.]*)");
            Match drc4 = Regex.Match(input, @"-D ([0-9.]*),([0-9.]*),([0-9.]*),([0-9.]*)");

            Match subtitles = Regex.Match(input, @"-s ([0-9a-zA-Z]*)");
            Match subScan = Regex.Match(input, @" -U");
            Match forcedSubtitles = Regex.Match(input, @" -F");

            //H264 Tab
            Match x264 = Regex.Match(input, @"-x ([.,/a-zA-Z0-9=:-]*)");

            //Program Options
            Match verbose = Regex.Match(input, @" -v");
            #endregion

            #region Set Varibles
            try
            {
      
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

                #region Destination
                thisQuery.q_destination = destination.ToString().Replace("-o ", "").Replace("\"", "");

                string videoEncoderConvertion;

                videoEncoderConvertion = videoEncoder.ToString().Replace("-e ", "");
                switch (videoEncoderConvertion)
                {
                    case "ffmpeg":
                        videoEncoderConvertion = "MPEG-4 (FFmpeg)";
                        break;
                    case "xvid":
                        videoEncoderConvertion = "MPEG-4 (XviD)";
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

                #endregion

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
                    switch (denoise.ToString().Replace("--denoise=", "").Replace("\"", ""))
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
     
                #region Video Settings Tab
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
                    qConvert = double.Parse(videoQuality.ToString().Replace("-q ", ""), Functions.Encode.Culture) * 100;
                    qConvert = System.Math.Ceiling(qConvert);
                    thisQuery.q_videoQuality = int.Parse(qConvert.ToString());
                }
                thisQuery.q_ipodAtom = ipodAtom.Success;
                thisQuery.q_optimizeMp4 = optimizeMP4.Success;

                #endregion

                #region Audio Tab

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

                if (audioTrack3.Success != false)
                {
                    string[] audioChan = audioTrack3.ToString().Split(',');
                    thisQuery.q_audioTrack3 = audioChan[2];
                }
                else
                    thisQuery.q_audioTrack3 = "None";

                if (audioTrack4.Success != false)
                {
                    string[] audioChan = audioTrack4.ToString().Split(',');
                    thisQuery.q_audioTrack4 = audioChan[3];
                }
                else
                    thisQuery.q_audioTrack4 = "None";

    
                // Mixdowns
                thisQuery.q_audioTrackMix1 = "Automatic";
                if (audioTrack1Mix.Success != false)
                {
                    thisQuery.q_audioTrackMix1 = getMixDown(audioTrack1Mix.ToString().Replace("-6 ", "").Replace(" ", ""));
                }

                thisQuery.q_audioTrackMix2 = "Automatic";
                if (audioTrack2Mix.Success != false)
                {
                    string[] audio2mix = audioTrack2Mix.ToString().Split(',');
                    audio2mix[1] = audio2mix[1].Trim();
                    thisQuery.q_audioTrackMix2 = getMixDown(audio2mix[1]);
                }

                thisQuery.q_audioTrackMix3 = "Automatic";
                if (audioTrack3Mix.Success != false)
                {
                    string[] audio3mix = audioTrack3Mix.ToString().Split(',');
                    audio3mix[1] = audio3mix[2].Trim();
                    thisQuery.q_audioTrackMix3 = getMixDown(audio3mix[1]);
                }

                thisQuery.q_audioTrackMix4 = "Automatic";
                if (audioTrack4Mix.Success != false)
                {
                    string[] audio4mix = audioTrack4Mix.ToString().Split(',');
                    audio4mix[1] = audio4mix[3].Trim();
                    thisQuery.q_audioTrackMix4 = getMixDown(audio4mix[1]);
                }
                

                // Audio Encoders
                if (audioEncoder1.Success != false)
                    thisQuery.q_audioEncoder1 = getAudioEncoder(audioEncoder1.ToString().Replace("-E ", ""));

                if (audioEncoder2.Success != false)
                {
                    string[] audio2enc = audioEncoder2.ToString().Split(',');
                    thisQuery.q_audioEncoder2 = getAudioEncoder(audio2enc[1].Trim());
                }

                if (audioEncoder3.Success != false)
                {
                    string[] audio3enc = audioEncoder3.ToString().Split(',');
                    thisQuery.q_audioEncoder3 = getAudioEncoder(audio3enc[2].Trim());
                }

                if (audioEncoder4.Success != false)
                {
                    string[] audio4enc = audioEncoder4.ToString().Split(',');
                    thisQuery.q_audioEncoder4 = getAudioEncoder(audio4enc[3].Trim());
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

                if (audioBitrate3.Success != false)
                {
                    string[] audioBitrateSelect = audioBitrate3.ToString().Split(',');
                    thisQuery.q_audioBitrate3 = audioBitrateSelect[2].Trim();
                }
                else
                    thisQuery.q_audioBitrate3 = "";

                if (audioBitrate4.Success != false)
                {
                    string[] audioBitrateSelect = audioBitrate4.ToString().Split(',');
                    thisQuery.q_audioBitrate4 = audioBitrateSelect[3].Trim();
                }
                else
                    thisQuery.q_audioBitrate4 = "";


                // Audio Sample Rate
                // Make sure to change 0 to Auto
                if (audioSampleRate1.Success != false)
                {
                    thisQuery.q_audioSamplerate1 = audioSampleRate1.ToString().Replace("-R ", "").Trim();
                    if (thisQuery.q_audioSamplerate1 == "0")
                        thisQuery.q_audioSamplerate1 = "Auto";
                }
                else
                    thisQuery.q_audioSamplerate1 = "Auto";

                if (audioSampleRate2.Success != false)
                {
                    string[] audioSRSelect = audioSampleRate2.ToString().Split(',');
                    if (audioSRSelect[1] == "0")
                        audioSRSelect[1] = "Auto";
                    thisQuery.q_audioSamplerate2 = audioSRSelect[1].Trim();
                }

                if (audioSampleRate3.Success != false)
                {
                    string[] audioSRSelect = audioSampleRate3.ToString().Split(',');
                    if (audioSRSelect[1] == "0")
                        audioSRSelect[1] = "Auto";
                    thisQuery.q_audioSamplerate3 = audioSRSelect[2].Trim();
                }

                if (audioSampleRate4.Success != false)
                {
                    string[] audioSRSelect = audioSampleRate4.ToString().Split(',');
                    if (audioSRSelect[1] == "0")
                        audioSRSelect[1] = "Auto";
                    thisQuery.q_audioSamplerate4 = audioSRSelect[3].Trim();
                }

                // DRC
                if (drc1.Success != false)
                {
                    string value = drc1.ToString().Replace("-D ", "");
                    float drcValue = float.Parse(value);
                    drcValue = drcValue * 10;
                    thisQuery.q_drc1 = drcValue;
                }
                else
                    thisQuery.q_drc1 = 0;

                if (drc2.Success != false)
                {
                    string[] drcPoint = drc2.ToString().Split(',');
                    string value = drcPoint[1];
                    float drcValue = float.Parse(value);
                    drcValue = drcValue * 10;
                    thisQuery.q_drc2 = drcValue;
                }
                else
                    thisQuery.q_drc2 = 0;

                if (drc3.Success != false)
                {
                    string[] drcPoint = drc3.ToString().Split(',');
                    string value = drcPoint[2];
                    float drcValue = float.Parse(value);
                    drcValue = drcValue * 10;
                    thisQuery.q_drc3 = drcValue;
                }
                else
                    thisQuery.q_drc3 = 0;

                if (drc4.Success != false)
                {
                    string[] drcPoint = drc4.ToString().Split(',');
                    string value = drcPoint[3];
                    float drcValue = float.Parse(value);
                    drcValue = drcValue * 10;
                    thisQuery.q_drc4 = drcValue;
                }
                else
                    thisQuery.q_drc4 = 0;


                // Subtitle Stuff
                if (subtitles.Success != false)
                    thisQuery.q_subtitles = subtitles.ToString().Replace("-s ", "");
                else
                {
                    if (subScan.Success)
                        thisQuery.q_subtitles = "Autoselect";
                    else
                        thisQuery.q_subtitles = "None";
                }

                thisQuery.q_forcedSubs = forcedSubtitles.Success;

                #endregion

                #region H.264 and other
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