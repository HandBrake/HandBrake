/*  QueryParser.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

namespace HandBrake.ApplicationServices.Utilities
{
    using System;
    using System.Collections;
    using System.Collections.Generic;
    using System.Globalization;
    using System.Text.RegularExpressions;

    using HandBrake.ApplicationServices.Functions;
    using HandBrake.ApplicationServices.Model;
    using HandBrake.ApplicationServices.Model.Encoding;

    /// <summary>
    /// Parse a CLI Query
    /// </summary>
    public class QueryParserUtility
    {

        /**
         * TODO
         * - Add support for PointToPointMode = Seconds or Frames
         **/

        /// <summary>
        /// The Culture
        /// </summary>
        private static readonly CultureInfo Culture = new CultureInfo("en-US", false);

        /// <summary>
        /// Takes in a query which can be in any order and parses it. 
        /// All varibles are then set so they can be used elsewhere.
        /// </summary>
        /// <param name="input">A ClI Query</param>
        /// <returns>A Parsed Query</returns>
        public static EncodeTask Parse(string input)
        {
            var parsed = new EncodeTask();

            #region Regular Expressions

            // Source
            Match title = Regex.Match(input, @"-t ([0-9]*)");
            Match chapters = Regex.Match(input, @"-c ([0-9-]*)");

            // Output Settings
            Match format = Regex.Match(input, @"-f ([a-zA-Z0-9]*)");
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
            Match twoPass = Regex.Match(input, @" -2");
            Match turboFirstPass = Regex.Match(input, @" -T");
            Match optimizeMP4 = Regex.Match(input, @" -O");
            Match pfr = Regex.Match(input, @" --pfr");
            Match vfr = Regex.Match(input, @" --vfr");
            Match cfr = Regex.Match(input, @" --cfr");

            // Audio Settings Tab
            Match noAudio = Regex.Match(input, @"-a none");
            Match audioTracks = Regex.Match(input, @"-a ([0-9,]*)");
            Match audioTrackMixes = Regex.Match(input, @"-6 ([0-9a-zA-Z,]*)");
            Match audioEncoders = Regex.Match(input, @"-E ([a-zA-Z0-9+,:]*)");
            Match audioBitrates = Regex.Match(input, @"-B ([0-9a-zA-Z,]*)"); // Auto = a-z
            Match audioSampleRates = Regex.Match(input, @"-R ([0-9a-zA-Z.,]*)"); // Auto = a-z
            Match drcValues = Regex.Match(input, @"-D ([0-9.,]*)");

            // Chapters Tab
            Match chapterMarkers = Regex.Match(input, @" -m");
            Match chapterMarkersFileMode = Regex.Match(input, @"--markers");

            // Advanced Tab
            Match advanced = Regex.Match(input, @"-x ([.,/a-zA-Z0-9=:-]*)");

            #endregion

            #region Set Varibles

            try
            {
                #region Source Tab

                if (title.Success)
                {
                    parsed.Title = int.Parse(title.ToString().Replace("-t ", string.Empty));
                }

                if (chapters.Success)
                {
                    parsed.PointToPointMode = PointToPointMode.Chapters;
                    string[] actTitles = chapters.ToString().Replace("-c ", string.Empty).Split('-');
                    parsed.StartPoint = int.Parse(actTitles[0]);
                    if (actTitles.Length > 1)
                    {
                        parsed.EndPoint = int.Parse(actTitles[1]);
                    }

                    if ((parsed.StartPoint == 1) && (parsed.EndPoint == 0))
                    {
                        parsed.EndPoint = parsed.StartPoint;
                    }
                }
             
                #endregion

                #region Output Settings

                if (format.Success)
                {
                    parsed.OutputFormat = Converters.GetFileFormat(format.Groups[1].ToString());
                }
                parsed.LargeFile = largerMp4.Success;
                parsed.IPod5GSupport = ipodAtom.Success;
                parsed.OptimizeMP4 = optimizeMP4.Success;

                #endregion

                #region Picture Tab

                if (width.Success)
                    parsed.Width = int.Parse(width.Groups[0].Value.Replace("-w ", string.Empty));

                if (height.Success)
                    parsed.Height = int.Parse(height.Groups[0].Value.Replace("-l ", string.Empty));

                if (maxWidth.Success)
                    parsed.MaxWidth = int.Parse(maxWidth.Groups[0].Value.Replace("-X ", string.Empty));

                if (maxHeight.Success)
                    parsed.MaxHeight = int.Parse(maxHeight.Groups[0].Value.Replace("-Y ", string.Empty));

                if (crop.Success)
                {
                    try
                    {
                        string values = crop.ToString().Replace("--crop ", string.Empty);
                        string[] actCropValues = values.Split(':');
                        parsed.Cropping = new Cropping(
                            int.Parse(actCropValues[0]),
                            int.Parse(actCropValues[1]),
                            int.Parse(actCropValues[2]),
                            int.Parse(actCropValues[3]));
                    }
                    catch (Exception)
                    {
                        // No need to do anything.
                    }
                }

                if (strictAnamorphic.Success)
                    parsed.Anamorphic = Anamorphic.Strict;
                else if (looseAnamorphic.Success)
                    parsed.Anamorphic = Anamorphic.Loose;
                else if (customAnamorphic.Success)
                    parsed.Anamorphic = Anamorphic.Custom;
                else
                    parsed.Anamorphic = Anamorphic.None;

                parsed.KeepDisplayAspect = keepDisplayAsect.Success;

                if (displayWidth.Success)
                    parsed.DisplayWidth =
                        double.Parse(displayWidth.Groups[0].Value.Replace("--display-width ", string.Empty));

                if (pixelAspect.Success)
                    parsed.PixelAspectX = int.Parse(pixelAspect.Groups[1].Value.Replace("--pixel-aspect ", string.Empty));

                if (pixelAspect.Success && pixelAspect.Groups.Count >= 3)
                    parsed.PixelAspectY = int.Parse(pixelAspect.Groups[2].Value.Replace("--pixel-aspect ", string.Empty));

                if (modulus.Success)
                    parsed.Modulus = int.Parse(modulus.Groups[0].Value.Replace("--modulus ", string.Empty));

                #endregion

                #region Filters

                parsed.Decomb = Decomb.Off;
                if (decomb.Success)
                {
                    parsed.Decomb = Decomb.Default;
                    if (decombValue.Success)
                    {
                        parsed.CustomDecomb = decombValue.ToString().Replace("--decomb=", string.Empty).Replace("\"", string.Empty);
                    }
                }

                parsed.Deinterlace = Deinterlace.Off;
                if (deinterlace.Success)
                {
                    switch (deinterlace.ToString().Replace("--deinterlace=", string.Empty).Replace("\"", string.Empty).ToLower())
                    {
                        case "fast":
                            parsed.Deinterlace = Deinterlace.Fast;
                            break;
                        case "slow":
                            parsed.Deinterlace = Deinterlace.Slow;
                            break;
                        case "slower":
                            parsed.Deinterlace = Deinterlace.Slower;
                            break;
                        case "slowest":
                            parsed.Deinterlace = Deinterlace.Slowest;
                            break;
                        default:
                            parsed.Deinterlace = Deinterlace.Custom;
                            parsed.CustomDeinterlace = deinterlace.ToString().Replace("--deinterlace=", string.Empty).Replace("\"", string.Empty).ToLower();
                            break;
                    }
                }

                parsed.Denoise = Denoise.Off;
                if (denoise.Success)
                {
                    switch (denoise.ToString().Replace("--denoise=", string.Empty).Replace("\"", string.Empty))
                    {
                        case "weak":
                            parsed.Denoise = Denoise.Weak;
                            break;
                        case "medium":
                            parsed.Denoise = Denoise.Medium;
                            break;
                        case "strong":
                            parsed.Denoise = Denoise.Strong;
                            break;
                        default:
                            parsed.Denoise = Denoise.Custom;
                            parsed.CustomDenoise = denoise.ToString().Replace("--denoise=", string.Empty).Replace("\"", string.Empty);
                            break;
                    }
                }

                parsed.Deblock = 0;
                if (deblock.Success)
                {
                    int dval;
                    int.TryParse(deblock.ToString().Replace("--deblock=", string.Empty), out dval);
                    parsed.Deblock = dval;
                }

                parsed.Detelecine = Detelecine.Off;
                if (detelecine.Success)
                {
                    parsed.Detelecine = Detelecine.Default;
                    if (detelecineValue.Success)
                    {
                        parsed.CustomDetelecine = detelecineValue.ToString().Replace("--detelecine=", string.Empty).Replace("\"", string.Empty);
                        parsed.Detelecine = Detelecine.Custom;
                    }
                }

                #endregion

                #region Video Settings Tab

                parsed.VideoEncoder = Converters.GetVideoEncoder(videoEncoder.ToString().Replace("-e ", string.Empty));

                if (videoFramerate.Success)
                {
                    double fps;
                    double.TryParse(videoFramerate.Groups[1].ToString(), out fps);
                    parsed.Framerate = fps;
                }

                if (pfr.Success)
                    parsed.FramerateMode = FramerateMode.PFR;
                else if (cfr.Success)
                    parsed.FramerateMode = FramerateMode.CFR;
                else
                    parsed.FramerateMode = FramerateMode.VFR; // Default to VFR

                parsed.Grayscale = grayscale.Success;
                parsed.TwoPass = twoPass.Success;
                parsed.TurboFirstPass = turboFirstPass.Success;

                if (videoBitrate.Success)
                {
                    parsed.VideoEncodeRateType = VideoEncodeRateMode.AverageBitrate;
                    parsed.VideoBitrate = int.Parse(videoBitrate.ToString().Replace("-b ", string.Empty));
                }

                if (videoQuality.Success)
                {
                    float quality = float.Parse(videoQuality.ToString().Replace("-q ", string.Empty), Culture);
                    parsed.Quality = quality;
                }

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
                List<AudioTrack> allAudioTrackInfo = new List<AudioTrack>();
                for (int x = 0; x < encoderCount; x++)
                {
                    AudioTrack track = new AudioTrack();
                    if (trackData != null)
                        if (trackData.Length >= (x + 1)) // Audio Track
                            track.Track = int.Parse(trackData[x].Trim());

                    if (trackMixes != null)
                        if (trackMixes.Length >= (x + 1)) // Audio Mix
                            track.MixDown = Converters.GetAudioMixDown(Converters.GetMixDown(trackMixes[x].Trim()));

                    if (trackEncoders != null)
                        if (trackEncoders.Length >= (x + 1)) // Audio Mix
                            track.Encoder = Converters.GetAudioEncoderFromCliString(trackEncoders[x].Trim());

                    if (trackBitrates != null)
                        if (trackBitrates.Length >= (x + 1)) // Audio Encoder
                            track.Bitrate = int.Parse(trackBitrates[x].Trim() == "auto" ? "0" : trackBitrates[x].Trim());

                    if (trackSamplerates != null)
                        if (trackSamplerates.Length >= (x + 1)) // Audio SampleRate
                            track.SampleRate = double.Parse(trackSamplerates[x].Trim());

                    if (trackDRCvalues != null)
                        if (trackDRCvalues.Length >= (x + 1)) // Audio DRC Values
                            track.DRC = double.Parse(trackDRCvalues[x].Trim());

                    allAudioTrackInfo.Add(track);
                }

                parsed.AudioTracks = allAudioTrackInfo;

                #endregion

                #region Chapters Tab

                if (chapterMarkersFileMode.Success || chapterMarkers.Success)
                    parsed.IncludeChapterMarkers = true;

                #endregion

                #region Advanced and other

                if (advanced.Success)
                    parsed.AdvancedEncoderOptions = advanced.ToString().Replace("-x ", string.Empty);

                #endregion
            }
            catch (Exception exc)
            {
                throw new Exception("An error has occured in the QueryParser Utility.", exc);
            }

            #endregion

            return parsed;
        }
    }
}