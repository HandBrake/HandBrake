/*  QueryGeneratorUtility.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr/>.
    It may be used under the terms of the GNU General Public License. */

namespace HandBrake.ApplicationServices.Utilities
{
    using System;
    using System.Collections.Generic;
    using System.Collections.ObjectModel;
    using System.Globalization;
    using System.IO;

    using HandBrake.ApplicationServices.Functions;
    using HandBrake.ApplicationServices.Model;
    using HandBrake.ApplicationServices.Model.Encoding;
    using HandBrake.ApplicationServices.Services.Interfaces;
    using HandBrake.Interop.Model.Encoding;
    using HandBrake.Interop.Model.Encoding.x264;

    /// <summary>
    /// Generate a CLI Query for HandBrakeCLI
    /// </summary>
    public class QueryGeneratorUtility
    {
        /// <summary>
        /// Backing field for the user settings service.
        /// </summary>
        private static readonly IUserSettingService UserSettingService = ServiceManager.UserSettingService;

        /// <summary>
        /// Generate a CLI Query for an EncodeTask Model object
        /// </summary>
        /// <param name="task">
        /// The task.
        /// </param>
        /// <returns>
        /// A Cli Query
        /// </returns>
        public static string GenerateQuery(EncodeTask task)
        {
            string query = string.Empty;
            query += SourceQuery(task, null, null);
            query += DestinationQuery(task);
            query += GenerateTabbedComponentsQuery(task, true);

            return query;
        }

        /// <summary>
        /// Generate a Query for a Preview Encode
        /// </summary>
        /// <param name="task">
        /// The task.
        /// </param>
        /// <param name="duration">
        /// The duration.
        /// </param>
        /// <param name="startAtPreview">
        /// The start At Preview.
        /// </param>
        /// <returns>
        /// A Cli query suitable for generating a preview video.
        /// </returns>
        public static string GeneratePreviewQuery(EncodeTask task, int duration, string startAtPreview)
        {
            string query = string.Empty;
            query += SourceQuery(task, duration, startAtPreview);
            query += DestinationQuery(task);
            query += GenerateTabbedComponentsQuery(task, true);

            return query;
        }

        #region Individual Query Sections

        /// <summary>
        /// Generate a Query from an Encode Task Object.
        /// </summary>
        /// <param name="task">
        /// The task.
        /// </param>
        /// <param name="enableFilters">
        /// The enableFilters.
        /// </param>
        /// <returns>
        /// The CLI query for the Tabbed section of the main window UI
        /// </returns>
        private static string GenerateTabbedComponentsQuery(EncodeTask task, bool enableFilters)
        {
            string query = string.Empty;

            // Output Settings
            query += OutputSettingsQuery(task);

            // Filters Panel
            if (enableFilters)
                query += FiltersQuery(task);

            // Picture Settings
            query += PictureSettingsQuery(task);

            // Video Settings
            query += VideoSettingsQuery(task);

            // Audio Settings
            query += AudioSettingsQuery(task);

            // Subtitles Panel
            query += SubtitlesQuery(task);

            // Chapter Markers
            query += ChapterMarkersQuery(task);

            // Advanced Panel
            query += AdvancedQuery(task);

            // Extra Settings
            query += ExtraSettings();

            return query;
        }

        /// <summary>
        /// Generate the Command Line Arguments for the Source
        /// </summary>
        /// <param name="task">
        /// The encode task.
        /// </param>
        /// <param name="duration">
        /// The duration.
        /// </param>
        /// <param name="preview">
        /// The preview.
        /// </param>
        /// <returns>
        /// A Cli Query as a string
        /// </returns>
        private static string SourceQuery(EncodeTask task, int? duration, string preview)
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
                    query += " --previews " + UserSettingService.GetUserSetting<int>(ASUserSettingConstants.PreviewScanCount) + " ";
                    query += " --start-at-preview " + preview;
                    query += " --stop-at duration:" + duration + " ";
                    break;
            }

            return query;
        }

        /// <summary>
        /// Generate the Command Line Arguments for the Destination File
        /// </summary>
        /// <param name="task">
        /// The encode task.
        /// </param>
        /// <returns>
        /// A Cli Query as a string
        /// </returns>
        private static string DestinationQuery(EncodeTask task)
        {
            string query = string.Empty;

            if (task.PointToPointMode == PointToPointMode.Preview)
                query += string.Format(" -o \"{0}\" ", task.Destination.Replace(".m", "_sample.m"));
            else
                query += string.Format(" -o \"{0}\" ", task.Destination);

            return query;
        }

        /// <summary>
        /// Generate the Command Line Arguments for the Output Settings
        /// </summary>
        /// <param name="task">
        /// The encode task.
        /// </param>
        /// <returns>
        /// A Cli Query as a string
        /// </returns>
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

        /// <summary>
        /// Generate the Command Line Arguments for the Picture Settings tab
        /// </summary>
        /// <param name="task">
        /// The encode task.
        /// </param>
        /// <returns>
        /// A Cli Query as a string
        /// </returns>
        private static string PictureSettingsQuery(EncodeTask task)
        {
            string query = string.Empty;

            if (task.Anamorphic != Anamorphic.Strict)
            {
                if (task.MaxWidth.HasValue) query += string.Format(" -X {0}", task.MaxWidth);
                else if (task.Width.HasValue) query += string.Format(" -w {0}", task.Width);

                if (task.MaxWidth.HasValue) query += string.Format(" -Y {0}", task.MaxHeight);
                else if (task.Height.HasValue) query += string.Format(" -h {0}", task.Height);
            }

            if (task.HasCropping)
            {
                query += string.Format(" --crop {0}:{1}:{2}:{3}", task.Cropping.Top, task.Cropping.Bottom, task.Cropping.Left, task.Cropping.Right);
            }

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

            if (task.Modulus.HasValue)
            {
                query += " --modulus " + task.Modulus;
            }

            return query;
        }

        /// <summary>
        /// Generate the Command Line Arguments for the Filters Tab
        /// </summary>
        /// <param name="task">
        /// The encode task.
        /// </param>
        /// <returns>
        /// A Cli Query as a string
        /// </returns>
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

        /// <summary>
        /// Generate the Command Line Arguments for the Video Settings Tab
        /// </summary>
        /// <param name="task">
        /// The encode task.
        /// </param>
        /// <returns>
        /// A Cli Query as a string
        /// </returns>
        private static string VideoSettingsQuery(EncodeTask task)
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

            double x264CqStep = UserSettingService.GetUserSetting<double>(ASUserSettingConstants.X264Step);

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
                            query += string.Format(" -q {0}", value.ToString(CultureInfo.InvariantCulture));
                            break;
                        case VideoEncoder.X264:
                            value = 51 - (task.Quality.Value * x264CqStep);
                            value = Math.Round(value, 2);
                            query += string.Format(" -q {0}", value.ToString(CultureInfo.InvariantCulture));
                            break;
                        case VideoEncoder.Theora:
                            value = task.Quality.Value;
                            query += string.Format(" -q {0}", value.ToString(CultureInfo.InvariantCulture));
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

        /// <summary>
        /// Generate the Command Line Arguments for the Audio Settings Tab
        /// </summary>
        /// <param name="task">
        /// The encode task.
        /// </param>
        /// <returns>
        /// A Cli Query as a string
        /// </returns>
        private static string AudioSettingsQuery(EncodeTask task)
        {
            string query = string.Empty;

            ObservableCollection<AudioTrack> audioTracks = task.AudioTracks;

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

            // Passthru Settings
            if (task.AllowedPassthruOptions != null)
            {
                string fallbackEncoders = string.Empty;

                if (task.AllowedPassthruOptions.AudioAllowAACPass)
                {
                    fallbackEncoders += "aac";
                }

                if (task.AllowedPassthruOptions.AudioAllowAC3Pass)
                {
                    fallbackEncoders += string.IsNullOrEmpty(fallbackEncoders) ? "ac3" : ",ac3";
                }

                if (task.AllowedPassthruOptions.AudioAllowDTSHDPass)
                {
                    fallbackEncoders += string.IsNullOrEmpty(fallbackEncoders) ? "dtshd" : ",dtshd";
                }

                if (task.AllowedPassthruOptions.AudioAllowDTSPass)
                {
                    fallbackEncoders += string.IsNullOrEmpty(fallbackEncoders) ? "dts" : ",dts";
                }

                if (task.AllowedPassthruOptions.AudioAllowMP3Pass)
                {
                    fallbackEncoders += string.IsNullOrEmpty(fallbackEncoders) ? "mp3" : ",mp3";
                }

                if (!string.IsNullOrEmpty(fallbackEncoders))
                {
                    query += string.Format(" --audio-copy-mask {0}", fallbackEncoders);
                }

                query += string.Format(" --audio-fallback {0}", Converters.GetCliAudioEncoder(task.AllowedPassthruOptions.AudioEncoderFallback));
            }

            return query;
        }

        /// <summary>
        /// Generate the Command Line Arguments for the Subtitles Tab
        /// </summary>
        /// <param name="task">
        /// The encode task.
        /// </param>
        /// <returns>
        /// A Cli Query as a string
        /// </returns>
        private static string SubtitlesQuery(EncodeTask task)
        {
            string query = string.Empty;
            if (task.SubtitleTracks.Count != 0)
            {
                // BitMap and CC's
                string subtitleTracks = String.Empty;
                string subtitleForced = String.Empty;
                string subtitleBurn = String.Empty;
                string subtitleDefault = String.Empty;

                // SRT
                string srtFile = String.Empty;
                string srtCodeset = String.Empty;
                string srtOffset = String.Empty;
                string srtLang = String.Empty;
                string srtDefault = String.Empty;
                int srtCount = 0;
                int subCount = 0;

                // Languages
                IDictionary<string, string> langMap = LanguageUtilities.MapLanguages();

                foreach (SubtitleTrack item in task.SubtitleTracks)
                {
                    string itemToAdd;

                    if (item.IsSrtSubtitle) // We have an SRT file
                    {
                        srtCount++; // SRT track id.

                        srtLang += srtLang == string.Empty ? langMap[item.SrtLang] : "," + langMap[item.SrtLang];
                        srtCodeset += srtCodeset == string.Empty ? item.SrtCharCode : "," + item.SrtCharCode;

                        if (item.Default)
                            srtDefault = srtCount.ToString();

                        itemToAdd = item.SrtPath;
                        srtFile += srtFile == string.Empty ? itemToAdd : "," + itemToAdd;

                        itemToAdd = item.SrtOffset.ToString();
                        srtOffset += srtOffset == string.Empty ? itemToAdd : "," + itemToAdd;
                    }
                    else // We have Bitmap or CC
                    {
                        subCount++;

                        // Find --subtitle <string>
                        if (item.Track.Contains("Foreign Audio Search"))
                            itemToAdd = "scan";
                        else
                        {
                            string[] tempSub = item.Track.Split(' ');
                            itemToAdd = tempSub[0];
                        }

                        subtitleTracks += subtitleTracks == string.Empty ? itemToAdd : "," + itemToAdd;

                        // Find --subtitle-forced
                        if (item.Forced)
                            subtitleForced += subtitleForced == string.Empty ? subCount.ToString() : "," + subCount;

                        // Find --subtitle-burn
                        if (item.Burned)
                            subtitleBurn = subCount.ToString();

                        // Find --subtitle-default
                        if (item.Default)
                            subtitleDefault = subCount.ToString();
                    }
                }

                // Build The CLI Subtitles Query
                if (subtitleTracks != string.Empty)
                {
                    query += " --subtitle " + subtitleTracks;

                    if (subtitleForced != string.Empty)
                        query += " --subtitle-forced=" + subtitleForced;
                    if (subtitleBurn != string.Empty)
                        query += " --subtitle-burn=" + subtitleBurn;
                    if (subtitleDefault != string.Empty)
                        query += " --subtitle-default=" + subtitleDefault;
                }

                if (srtFile != string.Empty) // SRTs
                {
                    query += " --srt-file " + "\"" + srtFile + "\"";

                    if (srtCodeset != string.Empty)
                        query += " --srt-codeset " + srtCodeset;
                    if (srtOffset != string.Empty)
                        query += " --srt-offset " + srtOffset;
                    if (srtLang != string.Empty)
                        query += " --srt-lang " + srtLang;
                    if (srtDefault != string.Empty)
                        query += " --srt-default=" + srtDefault;
                }
            }

            return query;
        }

        /// <summary>
        /// Generate the Command Line Arguments for the Chapter markers tab
        /// </summary>
        /// <param name="task">
        /// The encode task.
        /// </param>
        /// <returns>
        /// A Cli Query as a string
        /// </returns>
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

        /// <summary>
        /// Generate the Command Line Arguments for the Advanced Encoder Options
        /// </summary>
        /// <param name="task">
        /// The encode task.
        /// </param>
        /// <returns>
        /// A Cli Query as a string
        /// </returns>
        private static string AdvancedQuery(EncodeTask task)
        {
            if (task.VideoEncoder == VideoEncoder.X264)
            {
                string query = string.Empty; 

                if (task.x264Preset != x264Preset.None)
                {
                    query += string.Format("--x264-preset={0} ", task.x264Preset.ToString().ToLower().Replace(" ", string.Empty));
                }

                if (task.x264Profile != x264Profile.None)
                {
                    query += string.Format("--x264-profile={0} ", task.x264Profile.ToString().ToLower().Replace(" ", string.Empty));
                }

                if (task.X264Tune != x264Tune.None)
                {
                    query += string.Format("--x264-tune={0} ", task.X264Tune.ToString().ToLower().Replace(" ", string.Empty));
                }

                if (!string.IsNullOrEmpty(task.AdvancedEncoderOptions))
                {
                    query += string.Format(" -x {0}", task.AdvancedEncoderOptions);
                }

                return query;
            }

            return string.IsNullOrEmpty(task.AdvancedEncoderOptions) ? string.Empty : string.Format(" -x {0}", task.AdvancedEncoderOptions);
        }

        /// <summary>
        /// Generate the Command Line Arguments for any additional advanced options.
        /// </summary>
        /// <returns>
        /// A Cli Query as a string
        /// </returns>
        private static string ExtraSettings()
        {
            string query = string.Empty;

            // Verbosity Level
            int verbosity = UserSettingService.GetUserSetting<int>(ASUserSettingConstants.Verbosity);
            query += string.Format(" --verbose= {0}", verbosity);

            // LibDVDNav
            if (UserSettingService.GetUserSetting<bool>(ASUserSettingConstants.DisableLibDvdNav))
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
        private static bool ChapterCsvSave(IEnumerable<ChapterMarker> chapters, string filePathName)
        {
            string csv = string.Empty;
            int counter = 0;

            foreach (ChapterMarker name in chapters)
            {
                csv += counter + "," + name.ChapterName.Replace(",", "\\,") + Environment.NewLine;
                counter++;
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