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
    using System.ComponentModel.Composition;
    using System.Linq;

    using Caliburn.Micro;

    using HandBrake.ApplicationServices.Model;
    using HandBrake.ApplicationServices.Model.Encoding;
    using HandBrake.ApplicationServices.Parsing;
    using HandBrake.ApplicationServices.Services.Interfaces;
    using HandBrake.ApplicationServices.Utilities;

    using HandBrakeWPF.ViewModels.Interfaces;

    using Ookii.Dialogs.Wpf;

    /// <summary>
    /// The Subtitles View Model
    /// </summary>
    [Export(typeof(ISubtitlesViewModel))]
    public class SubtitlesViewModel : ViewModelBase, ISubtitlesViewModel
    {
        #region Constants and Fields

        /// <summary>
        /// Backing field for the source subtitle tracks.
        /// </summary>
        private IEnumerable<Subtitle> sourceTracks;

        #endregion

        #region Constructors and Destructors

        /// <summary>
        /// Initializes a new instance of the <see cref="HandBrakeWPF.ViewModels.SubtitlesViewModel"/> class.
        /// </summary>
        /// <param name="windowManager">
        /// The window manager.
        /// </param>
        /// <param name="userSettingService">
        /// The user Setting Service.
        /// </param>
        public SubtitlesViewModel(IWindowManager windowManager, IUserSettingService userSettingService)
        {
            this.Task = new EncodeTask();

            this.Langauges = LanguageUtilities.MapLanguages().Keys;
            this.CharacterCodes = CharCodesUtilities.GetCharacterCodes();
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
        public IEnumerable<Subtitle> SourceTracks
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
            this.AddAllClosedCaptions();
        }

        /// <summary>
        /// Import an SRT File.
        /// </summary>
        public void Import()
        {
            VistaOpenFileDialog dialog = new VistaOpenFileDialog
                {
                   Filter = "SRT files (*.srt)|*.srt", CheckFileExists = true, Multiselect = true 
                };

            dialog.ShowDialog();

            foreach (var srtFile in dialog.FileNames)
            {
                SubtitleTrack track = new SubtitleTrack
                    {
                        SrtFileName = srtFile, 
                        SrtOffset = 0, 
                        SrtCharCode = "UTF-8", 
                        SrtLang = "English", 
                        SubtitleType = SubtitleType.SRT
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
            this.Task = task;
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
            this.SourceTracks = title.Subtitles;
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
            if (this.SourceTracks != null)
            {
                string preferred =
                    this.UserSettingService.GetUserSetting<string>(UserSettingConstants.NativeLanguageForSubtitles);

                Subtitle source = subtitle ??
                                  (this.SourceTracks.FirstOrDefault(l => l.Language == preferred) ??
                                   this.SourceTracks.FirstOrDefault());

                if (source != null)
                {
                    SubtitleTrack track = new SubtitleTrack
                        {
                           SubtitleType = SubtitleType.VobSub, SourceTrack = source, 
                        };

                    this.Task.SubtitleTracks.Add(track);
                }
            }
        }

        /// <summary>
        /// Add all closed captions not already on the list.
        /// </summary>
        private void AddAllClosedCaptions()
        {
            if (this.UserSettingService.GetUserSetting<bool>(UserSettingConstants.UseClosedCaption))
            {
                foreach (
                    Subtitle subtitle in this.SourceTitlesSubset(null).Where(s => s.SubtitleType == SubtitleType.CC))
                {
                    this.Add(subtitle);
                }
            }
        }

        /// <summary>
        /// Add all the remaining subtitle tracks.
        /// </summary>
        private void AddAllRemaining()
        {
            foreach (Subtitle subtitle in this.SourceTitlesSubset(null))
            {
                this.Add(subtitle);
            }
        }

        /// <summary>
        /// Add all remaining tracks for the users preferred and selected languages
        /// </summary>
        private void AddAllRemainingForSelectedLanguages()
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
                       ? subtitles.Where(
                           subtitle => !this.Task.SubtitleTracks.Any(track => track.SourceTrack == subtitle)).ToList()
                       : this.SourceTracks.Where(
                           subtitle => !this.Task.SubtitleTracks.Any(track => track.SourceTrack == subtitle)).ToList();
        }

        #endregion
    }
}