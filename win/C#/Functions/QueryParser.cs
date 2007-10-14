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

        private string q_dvdChapters;
        /// <summary>
        /// Returns an String
        /// DVD Chapter number or chapter range.
        /// </summary>
        public string DVDChapters
        {
            get
            {
                return this.q_dvdChapters;
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

        private string q_audioEncoder;
        /// <summary>
        /// Returns an String
        /// The Audio Encoder used.
        /// </summary>
        public string AudioEncoder
        {
            get
            {
                return this.q_audioEncoder;
            }
        }

        private int q_videoWidth;
        /// <summary>
        /// Returns an Integer
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
        /// Returns an Integer
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

        private string  q_videoTargetSize;
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

        private Boolean q_crf;
        /// <summary>
        /// Returns a boolean to indicate if CRF is on or off.
        /// </summary>
        public Boolean CRF
        {
            get
            {
                return this.q_crf;
            }
        }

        private string q_audioBitrate;
        /// <summary>
        /// Returns a string with the audio bitrate
        /// </summary>
        public string AudioBitrate
        {
            get
            {
                return this.q_audioBitrate;
            }
        }

        private string q_audioSamplerate;
        /// <summary>
        /// Returns a string with the audio sample rate
        /// </summary>
        public string AudioSampleBitrate
        {
            get
            {
                return this.q_audioSamplerate;
            }
        }

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

        private string q_audioTrackMix;
        /// <summary>
        /// Returns a string with the First selected Audio track Mix
        /// </summary>
        public string AudioTrackMix
        {
            get
            {
                return this.q_audioTrackMix;
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

        // Takes in a query which can be in any order and parses it. All varibles are then set so they can be used elsewhere.
        public static QueryParser Parse(String input)
        {
            QueryParser thisQuery = new QueryParser();

            // Example Input
            // -i "C:\Documents and Settings\Scott\Desktop\Files\DVD\Test Image.iso" 
            //-t 1 -c 3-5 -o "C:\test.mp4" -e x264 -E faac -w 720 -l 400 --crop 0:0:0:0
            //-s 1 --deinterlace="1:-1:1" -g  -m  -b 2500 -2  -r 25 -T  -4  --deblock --detelecine 
            //--denoise=7:7:5:5 -x bframes=3:ref=1:subme=5:me=umh:no-fast-pskip=1:trellis=2 -B 160 -R 48 -a 1 -6 stereo -v 
            //-q 0.45 -Q -S 2134


            //Source
            Regex r1 = new Regex(@"(-i)(?:\s\"")([a-zA-Z0-9:\\\s\.]+)(?:\"")");
            Match source = r1.Match(input.Replace('"', '\"'));
            Match title = Regex.Match(input, @"-t ([0-9]*)");
            Match chapters = Regex.Match(input, @"-c ([0-9-]*)");

            //Destination
            Regex r2 = new Regex(@"(-o)(?:\s\"")([a-zA-Z0-9:\\\s\.]+)(?:\"")");
            Match destination = r2.Match(input.Replace('"', '\"'));
            Match width = Regex.Match(input, @"-w ([0-9]*)");
            Match height = Regex.Match(input, @"-l ([0-9]*)");
            Match videoEncoder = Regex.Match(input, @"-e ([a-zA-Z0-9]*)");
            Match audioEncoder = Regex.Match(input, @"-E ([a-zA-Z0-9]*)");

            //Picture Settings Tab
            Match deinterlace = Regex.Match(input, @"--deinterlace=([0-9:-]*)");  // DOES NOT WORK. Needs Fixed
            Match denoise = Regex.Match(input, @"--denoise=([0-9:]*)");
            Match deblock = Regex.Match(input, @"--deblock");
            Match detelecine = Regex.Match(input, @"--detelecine");
            Match anamorphic = Regex.Match(input, @"-p");
            Match chapterMarkers = Regex.Match(input, @"-m");
            Match crop = Regex.Match(input, @"--crop ([0-9]):([0-9]):([0-9]):([0-9])");

            //Video Settings Tab
            Match videoFramerate = Regex.Match(input, @"-r ([0-9]*)");
            Match videoBitrate = Regex.Match(input, @"-b ([0-9]*)");
            Match videoQuality = Regex.Match(input, @"-q ([0-9.]*)");
            Match videoFilesize = Regex.Match(input, @"-S ([0-9.]*)");
            Match CRF = Regex.Match(input, @"-Q");
            Match twoPass = Regex.Match(input, @"-2");
            Match turboFirstPass = Regex.Match(input, @"-T");
            Match grayscale = Regex.Match(input, @"-g");
            Match largerMp4 = Regex.Match(input, @"-4");

            //Audio Settings Tab
            Match subtitles = Regex.Match(input, @"-s ([0-9]*)");
            Match audioBitrate = Regex.Match(input, @"-B ([0-9]*)");
            Match audioSampleRate = Regex.Match(input, @"-R ([0-9.]*)");
            Match audioChannelsMix = Regex.Match(input, @"-6 ([a-zA-Z0-9]*)");
            Match audioChannel = Regex.Match(input, @"-a ([0-9]*)");

            //H264 Tab
            Match x264 = Regex.Match(input, @"-x ([a-zA-Z0-9=:-]*)");
            
            //Program Options
            Match verbose = Regex.Match(input, @"-v");
             
            
            // ### NOTES ###
            // The following code needs alot of error handling added to it at some point.
            // May be an idea to add additional options such as CPU etc later.

            try
            {
                //Source
                thisQuery.q_source = source.ToString();
                thisQuery.q_dvdTitle = int.Parse(title.ToString().Replace("-t ", ""));
                thisQuery.q_dvdChapters = chapters.ToString();

                //Destination
                thisQuery.q_destination = destination.ToString();
                thisQuery.q_videoEncoder = videoEncoder.ToString();
                thisQuery.q_audioEncoder = audioEncoder.ToString();
                thisQuery.q_videoWidth = int.Parse(width.ToString().Replace("-w ", ""));
                thisQuery.q_videoHeight = int.Parse(height.ToString().Replace("-l ", ""));

                //Picture Settings Tab
                thisQuery.q_cropValues = crop.ToString();
                thisQuery.q_detelecine = detelecine.Success;
                thisQuery.q_deBlock = deblock.Success;
                thisQuery.q_deinterlace = deinterlace.ToString();
                thisQuery.q_denoise = denoise.ToString();
                thisQuery.q_anamorphic = anamorphic.Success;
                thisQuery.q_chapterMarkers = chapterMarkers.Success;

                //Video Settings Tab
                thisQuery.q_grayscale = grayscale.Success;
                thisQuery.q_twoPass = twoPass.Success;
                thisQuery.q_turboFirst = turboFirstPass.Success;
                thisQuery.q_largeMp4 = largerMp4.Success;
                thisQuery.q_videoFramerate = videoFramerate.ToString();
                thisQuery.q_avgBitrate = videoBitrate.ToString();
                thisQuery.q_videoTargetSize = videoFilesize.ToString();
                thisQuery.q_videoQuality = int.Parse(videoQuality.ToString());
                thisQuery.q_crf = CRF.Success;

                //Audio Settings Tab
                thisQuery.q_audioBitrate = audioBitrate.ToString();
                thisQuery.q_audioSamplerate = audioSampleRate.ToString();
                thisQuery.q_audioTrack1 = audioChannel.ToString();
                thisQuery.q_audioTrackMix = audioChannelsMix.ToString();
                thisQuery.q_subtitles = subtitles.ToString();

                //H264 Tab
                thisQuery.q_h264 = x264.ToString();

                //Progam Options
                thisQuery.q_verbose = verbose.Success;
            }
            catch (Exception exc)
            {
                MessageBox.Show(exc.ToString());
            }

            return thisQuery;
        }
    }
}
