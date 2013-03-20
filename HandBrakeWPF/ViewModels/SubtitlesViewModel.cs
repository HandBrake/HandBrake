// --------------------------------------------------------------------------------------------------------------------
// <copyright file="SubtitlesViewModel.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The Subtitles View Model
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.ViewModels
{
    using System.Collections.Generic;
    using System.Collections.Specialized;
    using System.IO;
    using System.Linq;

    using HandBrake.ApplicationServices.Model;
    using HandBrake.ApplicationServices.Model.Encoding;
    using HandBrake.ApplicationServices.Parsing;
    using HandBrake.ApplicationServices.Utilities;

    using HandBrakeWPF.Commands;
    using HandBrakeWPF.Model;
    using HandBrakeWPF.ViewModels.Interfaces;

    using Ookii.Dialogs.Wpf;

    /// <summary>
    /// The Subtitles View Model
    /// </summary>
    public class SubtitlesViewModel : ViewModelBase, ISubtitlesViewModel
    {
        #region Constants and Fields

        /// <summary>
        /// Backing field for the source subtitle tracks.
        /// </summary>
        private IList<Subtitle> sourceTracks;

        /// <summary>
        /// The Foreign Audio Search Track
        /// </summary>
        private readonly Subtitle ForeignAudioSearchTrack;

        #endregion

        #region Constructors and Destructors

        /// <summary>
        /// Initializes a new instance of the <see cref="HandBrakeWPF.ViewModels.SubtitlesViewModel"/> class.
        /// </summary>
        public SubtitlesViewModel()
        {
            this.Task = new EncodeTask();

            this.Langauges = LanguageUtilities.MapLanguages().Keys;
            this.CharacterCodes = CharCodesUtilities.GetCharacterCodes();

            this.ForeignAudioSearchTrack = new Subtitle { SubtitleType = SubtitleType.ForeignAudioSearch, Language = "Foreign Audio Search (Bitmap)" };
            this.SourceTracks = new List<Subtitle> { this.ForeignAudioSearchTrack };
        }

        #endregion

        #region Properties

        /// <summary>
        /// Gets or sets CharacterCodes.
        /// </summary>
        public IEnumerable<string> CharacterCodes { get; set; }

        /// <summary>
        /// Gets or sets Langauges.
        /// </summary>
        public IEnumerable<string> Langauges { get; set; }

        /// <summary>
        /// Gets or sets SourceTracks.
        /// </summary>
        public IList<Subtitle> SourceTracks
        {
            get
            {
                return this.sourceTracks;
            }

            set
            {
                this.sourceTracks = value;
                this.NotifyOfPropertyChange(() => this.SourceTracks);
            }
        }

        /// <summary>
        /// Gets or sets Task.
        /// </summary>
        public EncodeTask Task { get; set; }

        #endregion

        #region Public Methods

        /// <summary>
        /// Add a new Track
        /// </summary>
        public void Add()
        {
            this.Add(null);
        }

        /// <summary>
        /// Add all closed captions not already on the list.
        /// </summary>
        public void AddAllClosedCaptions()
        {

            foreach (Subtitle subtitle in this.SourceTitlesSubset(null).Where(s => s.SubtitleType == SubtitleType.CC))
            {
                this.Add(subtitle);
            }
        }

        /// <summary>
        /// Add all the remaining subtitle tracks.
        /// </summary>
        public void AddAllRemaining()
        {
            foreach (Subtitle subtitle in this.SourceTitlesSubset(null))
            {
                this.Add(subtitle);
            }
        }

        /// <summary>
        /// Add all remaining tracks for the users preferred and selected languages
        /// </summary>
        public void AddAllRemainingForSelectedLanguages()
        {
            // Get a list of subtitle tracks that match the users lanaguages
            StringCollection userSelectedLanguages =
                this.UserSettingService.GetUserSetting<StringCollection>(UserSettingConstants.SelectedLanguages);
            userSelectedLanguages.Add(
                this.UserSettingService.GetUserSetting<string>(UserSettingConstants.NativeLanguageForSubtitles));
            List<Subtitle> availableTracks =
                this.SourceTracks.Where(subtitle => userSelectedLanguages.Contains(subtitle.Language)).ToList();

            foreach (Subtitle subtitle in this.SourceTitlesSubset(availableTracks))
            {
                this.Add(subtitle);
            }
        }

        /// <summary>
        /// Import an SRT File.
        /// </summary>
        public void Import()
        {
            VistaOpenFileDialog dialog = new VistaOpenFileDialog
                {
                    Filter = "SRT files (*.srt)|*.srt",
                    CheckFileExists = true,
                    Multiselect = true
                };

            dialog.ShowDialog();

            foreach (var srtFile in dialog.FileNames)
            {
                SubtitleTrack track = new SubtitleTrack
                    {
                        SrtFileName = Path.GetFileNameWithoutExtension(srtFile),
                        SrtOffset = 0,
                        SrtCharCode = "UTF-8",
                        SrtLang = "English",
                        SubtitleType = SubtitleType.SRT,
                        SrtPath = srtFile
                    };
                this.Task.SubtitleTracks.Add(track);
            }
        }

        /// <summary>
        /// Remove a Track
        /// </summary>
        /// <param name="track">
        /// The track.
        /// </param>
        public void Remove(SubtitleTrack track)
        {
            this.Task.SubtitleTracks.Remove(track);
        }

        /// <summary>
        /// Clear all Tracks
        /// </summary>
        public void Clear()
        {
            this.Task.SubtitleTracks.Clear();
        }

        /// <summary>
        /// Select the default subtitle track.
        /// </summary>
        /// <param name="subtitle">
        /// The subtitle.
        /// </param>
        public void SelectDefaultTrack(SubtitleTrack subtitle)
        {
            foreach (SubtitleTrack track in this.Task.SubtitleTracks)
            {
                if (track == subtitle)
                {
                    continue; // Skip the track the user selected.
                }
                track.Default = false;
            }

            this.NotifyOfPropertyChange(() => this.Task);
        }

        /// <summary>
        /// Select the burned in track.
        /// </summary>
        /// <param name="subtitle">
        /// The subtitle.
        /// </param>
        public void SelectBurnedInTrack(SubtitleTrack subtitle)
        {
            foreach (SubtitleTrack track in this.Task.SubtitleTracks)
            {
                if (track == subtitle)
                {
                    continue; // Skip the track the user selected.
                }
                track.Burned = false;
            }
            this.NotifyOfPropertyChange(() => this.Task);
        }

        /// <summary>
        /// Automatic Subtitle Selection based on user preferences.
        /// </summary>
        public void AutomaticSubtitleSelection()
        {
            this.Task.SubtitleTracks.Clear();

            // New DUB Settings
            int mode = this.UserSettingService.GetUserSetting<int>(UserSettingConstants.DubModeSubtitle);
            switch (mode)
            {
                case 1: // Adding all remaining subtitle tracks
                    this.AddAllRemaining();
                    break;
                case 2: // Adding only the first or preferred first subtitle track.
                    this.Add();
                    break;
                case 3: // Selected Languages Only
                    this.AddAllRemainingForSelectedLanguages();
                    break;
                case 4: // Prefered Only
                    this.AddForPreferredLanaguages(true);
                    break;
                case 5: // Prefered Only All
                    this.AddForPreferredLanaguages(false);
                    break;
            }

            // Add all closed captions if enabled.
            if (this.UserSettingService.GetUserSetting<bool>(UserSettingConstants.UseClosedCaption))
            {
                this.AddAllClosedCaptions();
            }
        }

        /// <summary>
        /// Open the options screen to the Audio and Subtitles tab.
        /// </summary>
        public void SetDefaultBehaviour()
        {
            OpenOptionsScreenCommand command = new OpenOptionsScreenCommand();
            command.Execute(OptionsTab.AudioAndSubtitles);
        }

        #endregion

        #region Implemented Interfaces

        #region ITabInterface

        /// <summary>
        /// Setup this tab for the specified preset.
        /// </summary>
        /// <param name="preset">
        /// The preset.
        /// </param>
        /// <param name="task">
        /// The task.
        /// </param>
        public void SetPreset(Preset preset, EncodeTask task)
        {
            // Note, We don't support Subtitles in presets yet.
            this.Task = task;
            this.NotifyOfPropertyChange(() => this.Task);
        }

        /// <summary>
        /// Update all the UI controls based on the encode task passed in.
        /// </summary>
        /// <param name="task">
        /// The task.
        /// </param>
        public void UpdateTask(EncodeTask task)
        {
            this.Task = task;
            this.NotifyOfPropertyChange(() => this.Task.SubtitleTracks);
            this.NotifyOfPropertyChange(() => this.Task);
        }

        /// <summary>
        /// Setup this window for a new source
        /// </summary>
        /// <param name="title">
        /// The title.
        /// </param>
        /// <param name="preset">
        /// The preset.
        /// </param>
        /// <param name="task">
        /// The task.
        /// </param>
        public void SetSource(Title title, Preset preset, EncodeTask task)
        {
            this.SourceTracks.Clear();
            this.SourceTracks.Add(ForeignAudioSearchTrack);
            foreach (Subtitle subtitle in title.Subtitles)
            {
                this.SourceTracks.Add(subtitle);
            }

            this.Task = task;
            this.NotifyOfPropertyChange(() => this.Task);

            this.AutomaticSubtitleSelection();
        }

        #endregion

        #endregion

        #region Methods

        /// <summary>
        /// Add a subtitle track.
        /// The Source track is set based on the following order. If null, it will skip to the next option.
        ///   1. Passed in Subitle param
        ///   2. First preferred Subtitle from source
        ///   3. First subtitle from source.
        /// Will not add a subtitle if the source has none.
        /// </summary>
        /// <param name="subtitle">
        /// The subtitle. Use null to add preferred, or first from source (based on user preference)
        /// </param>
        private void Add(Subtitle subtitle)
        {
            string preferred =
                this.UserSettingService.GetUserSetting<string>(UserSettingConstants.NativeLanguageForSubtitles);

            Subtitle source = subtitle ??
                              ((this.SourceTracks != null)
                                   ? (this.SourceTracks.FirstOrDefault(l => l.Language == preferred) ??
                                      this.SourceTracks.FirstOrDefault(s => s.SubtitleType != SubtitleType.ForeignAudioSearch))
                                   : null);

            if (source == null)
            {
                source = ForeignAudioSearchTrack;
            }

            SubtitleTrack track = new SubtitleTrack
                    {
                        SubtitleType = SubtitleType.VobSub,
                        SourceTrack = source,
                    };

            if (source.SubtitleType == SubtitleType.PGS &&
                this.Task != null &&
                (this.Task.OutputFormat == OutputFormat.Mp4 || this.Task.OutputFormat == OutputFormat.M4V))
            {
                this.SelectBurnedInTrack(track);
            }
            
            this.Task.SubtitleTracks.Add(track);
        }

        /// <summary>
        /// Add all tracks for the preferred languages settings.
        /// </summary>
        /// <param name="firstOnly">
        /// The first only.
        /// </param>
        private void AddForPreferredLanaguages(bool firstOnly)
        {
            string preferred =
                this.UserSettingService.GetUserSetting<string>(UserSettingConstants.NativeLanguageForSubtitles);

            foreach (Subtitle subtitle in this.SourceTitlesSubset(null))
            {
                if (subtitle.Language == preferred)
                {
                    this.Add(subtitle);
                    if (firstOnly)
                    {
                        break;
                    }
                }
            }
        }

        /// <summary>
        /// Gets a list of Source subtitle tracks that are not currently used.
        /// </summary>
        /// <param name="subtitles">
        /// The subtitles. (Optional).  If null, works on the full source subtitle collection
        /// </param>
        /// <returns>
        /// An IEnumerable collection of subtitles
        /// </returns>
        private IEnumerable<Subtitle> SourceTitlesSubset(IEnumerable<Subtitle> subtitles)
        {
            return subtitles != null
                       ? subtitles.Where(subtitle => !this.Task.SubtitleTracks.Any(track => Equals(track.SourceTrack, subtitle))).ToList()
                       : this.SourceTracks.Where(subtitle => !this.Task.SubtitleTracks.Any(track => Equals(track.SourceTrack, subtitle))).ToList();
        }

        #endregion
    }
}