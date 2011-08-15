/*  QueryGeneratorUtility.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr/>.
    It may be used under the terms of the GNU General Public License. */

namespace HandBrake.ApplicationServices.Utilities
{
    using System;
    using System.Collections.Generic;
    using System.Globalization;
    using System.IO;

    using HandBrake.ApplicationServices.Functions;
    using HandBrake.ApplicationServices.Model;
    using HandBrake.ApplicationServices.Model.Encoding;
    using HandBrake.Interop.Model.Encoding;

    using AudioEncoder = HandBrake.ApplicationServices.Model.Encoding.AudioEncoder;
    using Mixdown = HandBrake.ApplicationServices.Model.Encoding.Mixdown;

    /*
     * TODO:
     * 1. Handle Subittles
     * 2. Wireupt he Generate Query Methods.
     * 3. Handle Settings Better.
     * 4. Test and bug fix.
     */ 

    /// <summary>
    /// Generate a CLI Query for HandBrakeCLI
    /// </summary>
    public class QueryGeneratorUtility
    {
        public static string GenerateQuery()
        {
            throw new NotImplementedException("This class hasn't been finished yet.");
            string query = string.Empty;
            return query;

        }

        public static string GeneratePreviewQuery()
        {
            throw new NotImplementedException("This class hasn't been finished yet.");
            string query = string.Empty;
            return query;
        }

        #region Individual Query Sections

        private static string GenerateTabbedComponentsQuery(EncodeTask task, bool filters, int width, int height, int verbose, bool noDvdNav, double x264Step)
        {
            string query = string.Empty;

            // Output Settings
            query += OutputSettingsQuery(task);

            // Filters Panel
            if (filters)
                query += FiltersQuery(task);

            // Picture Settings
            query += PictureSettingsQuery(task, width, height);

            // Video Settings
            query += VideoSettingsQuery(task, x264Step);

            // Audio Settings
            query += AudioSettingsQuery(task);

            // Subtitles Panel
            query += SubtitlesQuery(task);

            // Chapter Markers
            query += ChapterMarkersQuery(task);

            // X264 Panel
            query += X264Query(task);

            // Extra Settings
            query += ExtraSettings(verbose.ToString(), noDvdNav);

            return query;
        }

        private static string SourceQuery(EncodeTask task, int mode, int duration, string preview, string previewTotal)
        {
            string query = string.Empty;

            query += string.Format(" -i \"{0}\"", task.Source);
            query += string.Format(" -t {0}", task.Title);
            query += string.Format(" --angle {0}", task.Angle);

            // Decide what part of the video we want to encode.
            switch (task.PointToPointMode)
            {
                case PointToPointMode.Chapters: // Chapters

                    if (task.StartPoint == task.EndPoint)
                        query += string.Format(" -c {0}", task.StartPoint);
                    else
                        query += string.Format(" -c {0}-{1}", task.StartPoint, task.EndPoint);
                    break;
                case PointToPointMode.Seconds: // Seconds
                    int calculatedDuration = task.EndPoint - task.StartPoint;
                    query += string.Format(" --start-at duration:{0} --stop-at duration:{1}", task.StartPoint, calculatedDuration);
                    break;
                case PointToPointMode.Frames: // Frames
                    calculatedDuration = task.EndPoint - task.StartPoint;
                    query += string.Format(" --start-at frame:{0} --stop-at frame:{1}", task.StartPoint, calculatedDuration);
                    break;
                case PointToPointMode.Preview: // Preview
                    query += " --previews " + previewTotal + " ";
                    query += " --start-at-preview " + preview;
                    query += " --stop-at duration:" + duration + " ";
                    break;
                default:
                    break;
            }

            return query;
        }

        private static string DestinationQuery(EncodeTask task)
        {
            string query = string.Empty;

            if (task.PointToPointMode == PointToPointMode.Preview)
                query += string.Format(" -o \"{0}\" ", task.Destination.Replace(".m", "_sample.m"));
            else
                query += string.Format(" -o \"{0}\" ", task.Destination);

            return query;
        }

        private static string OutputSettingsQuery(EncodeTask task)
        {
            string query = string.Empty;

            query += string.Format(" -f {0} ", EnumHelper<Enum>.GetDescription(task.OutputFormat).ToLower());

            // These are output settings features
            if (task.LargeFile)
                query += " -4 ";

            if (task.IPod5GSupport)
                query += " -I ";

            if (task.OptimizeMP4)
                query += " -O ";

            return query;
        }

        private static string PictureSettingsQuery(EncodeTask task, int width, int height)
        {
            string query = string.Empty;

            if (task.Anamorphic != Anamorphic.Strict)
            {
                if (task.MaxWidth.HasValue) query += string.Format(" -X {0}", task.MaxWidth);
                else if (task.Width.HasValue) query += string.Format(" -w {0}", task.Width);

                if (task.MaxWidth.HasValue) query += string.Format(" -Y {0}", task.MaxHeight);
                else if (task.Height.HasValue) query += string.Format(" -h {0}", task.Height);
            }

            query += string.Format(" --crop {0}:{1}:{2}:{3}", task.Cropping.Top, task.Cropping.Bottom, task.Cropping.Left, task.Cropping.Right);

            switch (task.Anamorphic)
            {
                case Anamorphic.Strict:
                    query += " --strict-anamorphic ";
                    break;
                case Anamorphic.Loose:
                    query += " --loose-anamorphic ";
                    break;
                case Anamorphic.Custom:
                    query += " --custom-anamorphic ";

                    if (task.DisplayWidth.HasValue)
                        query += " --display-width " + task.DisplayWidth + " ";

                    if (task.KeepDisplayAspect)
                        query += " --keep-display-aspect ";

                    if (!task.KeepDisplayAspect)
                        query += string.Format(" --pixel-aspect {0}:{1}", task.PixelAspectX, task.PixelAspectY);
                    break;
            }
            query += " --modulus " + task.Modulus;

            return query;
        }

        private static string FiltersQuery(EncodeTask task)
        {

            string query = string.Empty;

            switch (task.Detelecine) // DeTelecine
            {
                case Detelecine.Off:
                    query += string.Empty;
                    break;
                case Detelecine.Default:
                    query += " --detelecine";
                    break;
                case Detelecine.Custom:
                    query += string.Format(" --detelecine=\"{0}\"", task.CustomDetelecine);
                    break;
                default:
                    query += string.Empty;
                    break;
            }

            switch (task.Decomb) // Decomb
            {
                case Decomb.Off:
                    query += string.Empty;
                    break;
                case Decomb.Default:
                    query += " --decomb";
                    break;
                case Decomb.Custom:
                    query += string.Format(" --decomb=\"{0}\"", task.CustomDecomb);
                    break;
                default:
                    query += string.Empty;
                    break;
            }

            switch (task.Deinterlace) // DeInterlace
            {
                case Deinterlace.Fast:
                    query += " --deinterlace=\"fast\"";
                    break;
                case Deinterlace.Slow:
                    query += " --deinterlace=\"slow\"";
                    break;
                case Deinterlace.Slower:
                    query += " --deinterlace=\"slower\"";
                    break;
                case Deinterlace.Custom:
                    query += string.Format(" --deinterlace=\"{0}\"", task.CustomDeinterlace);
                    break;
                default:
                    query += string.Empty;
                    break;
            }

            switch (task.Denoise) // Denoise
            {
                case Denoise.Weak:
                    query += " --denoise=\"weak\"";
                    break;
                case Denoise.Medium:
                    query += " --denoise=\"medium\"";
                    break;
                case Denoise.Strong:
                    query += " --denoise=\"strong\"";
                    break;
                case Denoise.Custom:
                    query += string.Format(" --denoise=\"{0}\"", task.CustomDenoise);
                    break;
                default:
                    query += string.Empty;
                    break;
            }

            if (task.Deblock != 4)
                query += string.Format(" --deblock={0}", task.Deblock);

            if (task.Grayscale)
                query += " -g ";

            return query;
        }

        private static string VideoSettingsQuery(EncodeTask task, double x264CqStep)
        {
            string query = string.Empty;

            switch (task.VideoEncoder)
            {
                case VideoEncoder.FFMpeg:
                    query += " -e ffmpeg";
                    break;
                case VideoEncoder.X264:
                    query += " -e x264";
                    break;
                case VideoEncoder.Theora:
                    query += " -e theora";
                    break;
                default:
                    query += " -e x264";
                    break;
            }

            switch (task.VideoEncodeRateType)
            {
                case VideoEncodeRateType.AverageBitrate:
                    if (task.VideoBitrate.HasValue)
                        query += string.Format(" -b {0}", task.VideoBitrate.Value);
                    break;
                case VideoEncodeRateType.ConstantQuality:
                    double value;
                    switch (task.VideoEncoder)
                    {
                        case VideoEncoder.FFMpeg:
                            value = 31 - (task.Quality.Value - 1);
                            query += string.Format(" -q {0}", value.ToString(new CultureInfo("en-US")));
                            break;
                        case VideoEncoder.X264:
                            CultureInfo culture = CultureInfo.CreateSpecificCulture("en-US");
                            value = 51 - (task.Quality.Value * x264CqStep);
                            value = Math.Round(value, 2);
                            query += string.Format(" -q {0}", value.ToString(culture));
                            break;
                        case VideoEncoder.Theora:
                            value = task.Quality.Value;
                            query += string.Format(" -q {0}", value.ToString(new CultureInfo("en-US")));
                            break;
                    }
                    break;

            }

            if (task.TwoPass)
                query += " -2 ";

            if (task.TurboFirstPass)
                query += " -T ";

            if (task.Framerate.HasValue)
                query += string.Format(" -r {0}", task.Framerate);

            switch (task.FramerateMode)
            {
                case FramerateMode.CFR:
                    query += " --cfr";
                    break;
                case FramerateMode.VFR:
                    query += " --vfr";
                    break;
                case FramerateMode.PFR:
                    query += " --pfr";
                    break;
                default:
                    query += " --vfr";
                    break;
            }

            return query;
        }

        private static string AudioSettingsQuery(EncodeTask task)
        {
            string query = string.Empty;

            List<AudioTrack> audioTracks = task.AudioTracks;

            List<int> tracks = new List<int>();
            List<AudioEncoder> codecs = new List<AudioEncoder>();
            List<Mixdown> mixdowns = new List<Mixdown>();
            List<double> samplerates = new List<double>();
            List<int> bitrates = new List<int>();
            List<double> drcs = new List<double>();

            // No Audio
            if (audioTracks.Count == 0)
                query += " -a none ";

            // Gather information about each audio track and store them in the declared lists.
            foreach (AudioTrack track in audioTracks)
            {
                if (track.Track == null)
                {
                    continue;
                }

                tracks.Add(track.Track.Value);

                // Audio Codec (-E)
                codecs.Add(track.Encoder);

                // Audio Mixdown (-6)
                mixdowns.Add(track.MixDown);

                // Sample Rate (-R)
                samplerates.Add(track.SampleRate);

                // Audio Bitrate (-B)
                bitrates.Add(track.Bitrate);

                // DRC (-D)
                drcs.Add(track.DRC);
            }

            // Audio Track (-a)
            string audioItems = string.Empty;
            bool firstLoop = true;

            foreach (int item in tracks)
            {
                if (firstLoop)
                {
                    audioItems = item.ToString();
                    firstLoop = false;
                }
                else
                    audioItems += "," + item;
            }
            if (audioItems.Trim() != String.Empty)
                query += " -a " + audioItems;
            firstLoop = true;
            audioItems = string.Empty; // Reset for another pass.

            // Audio Codec (-E)
            foreach (AudioEncoder item in codecs)
            {
                if (firstLoop)
                {
                    audioItems = Converters.GetCliAudioEncoder(item);
                    firstLoop = false;
                }
                else
                    audioItems += "," + item;
            }
            if (audioItems.Trim() != String.Empty)
                query += " -E " + audioItems;
            firstLoop = true;
            audioItems = string.Empty; // Reset for another pass.

            // Audio Mixdown (-6)
            foreach (Mixdown item in mixdowns)
            {
                if (firstLoop)
                {
                    audioItems = Converters.GetCliMixDown(item);
                    firstLoop = false;
                }
                else
                    audioItems += "," + item;
            }
            if (audioItems.Trim() != String.Empty)
                query += " -6 " + audioItems;
            firstLoop = true;
            audioItems = string.Empty; // Reset for another pass.

            // Sample Rate (-R)
            foreach (double item in samplerates)
            {
                if (firstLoop)
                {
                    audioItems = item.ToString();
                    firstLoop = false;
                }
                else
                    audioItems += "," + item;
            }
            if (audioItems.Trim() != String.Empty)
                query += " -R " + audioItems;
            firstLoop = true;
            audioItems = string.Empty; // Reset for another pass.

            // Audio Bitrate (-B)
            foreach (int item in bitrates)
            {
                if (firstLoop)
                {
                    audioItems = item.ToString();
                    firstLoop = false;
                }
                else
                    audioItems += "," + item;
            }
            if (audioItems.Trim() != String.Empty)
                query += " -B " + audioItems;
            firstLoop = true;
            audioItems = string.Empty; // Reset for another pass.

            // DRC (-D)
            foreach (var itm in drcs)
            {
                string item = itm.ToString(new CultureInfo("en-US"));
                if (firstLoop)
                {
                    audioItems = item;
                    firstLoop = false;
                }
                else
                    audioItems += "," + item;
            }
            if (audioItems.Trim() != String.Empty)
                query += " -D " + audioItems;

            return query;
        }

        private static string SubtitlesQuery(EncodeTask task)
        {
            // TODO
            return string.Empty;
        }

        private static string ChapterMarkersQuery(EncodeTask task)
        {
            string query = string.Empty;

            // Attach Source name and dvd title to the start of the chapters.csv filename.
            // This is for the queue. It allows different chapter name files for each title.
            string destName = Path.GetFileNameWithoutExtension(task.Destination);
            string sourceTitle = task.Title.ToString();

            if (task.IncludeChapterMarkers && destName != null)
            {
                if (destName.Trim() != String.Empty)
                {
                    string path = sourceTitle != "Automatic"
                                      ? Path.Combine(Path.GetTempPath(), destName + "-" + sourceTitle + "-chapters.csv")
                                      : Path.Combine(Path.GetTempPath(), destName + "-chapters.csv");

                    if (ChapterCsvSave(task.ChapterNames, path) == false)
                        query += " -m ";
                    else
                        query += " --markers=" + "\"" + path + "\"";
                }
                else
                    query += " -m";
            }

            return query;
        }

        private static string X264Query(EncodeTask task)
        {
            return string.IsNullOrEmpty(task.AdvancedEncoderOptions) ? string.Empty : string.Format(" -x {0}", task.AdvancedEncoderOptions);
        }

        private static string ExtraSettings(string verboseLevel, bool disableLibDvdNav)
        {
            string query = string.Empty;

            // Verbosity Level
            query += string.Format(" --verbose= {0}", string.IsNullOrEmpty(verboseLevel) ? "1" : verboseLevel);

            // LibDVDNav
            if (disableLibDvdNav)
                query += " --no-dvdnav";

            return query;
        }

        #endregion

        #region Helpers

        /// <summary>
        /// Create a CSV file with the data from the Main Window Chapters tab
        /// </summary>
        /// <param name="chapters">The List of chapters</param>
        /// <param name="filePathName">Path to save the csv file</param>
        /// <returns>True if successful </returns>
        private static bool ChapterCsvSave(List<string> chapters, string filePathName)
        {
            string csv = string.Empty;
            int counter = 0;

            foreach (string name in chapters)
            {
                csv += counter + "," + name.Replace(",", "\\,") + Environment.NewLine;
                counter ++;
            }


            StreamWriter file = new StreamWriter(filePathName);
            file.Write(csv);
            file.Close();
            file.Dispose();
            return true;
        }
        #endregion
    }
}