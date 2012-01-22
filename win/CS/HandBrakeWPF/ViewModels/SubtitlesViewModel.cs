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
    using System.Collections.ObjectModel;
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
        /// The subtitle tracks.
        /// </summary>
        private ObservableCollection<SubtitleTrack> subtitleTracks;

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
            this.SubtitleTracks = new ObservableCollection<SubtitleTrack>();

            Langauges = LanguageUtilities.MapLanguages().Keys;
            CharacterCodes = CharCodesUtilities.GetCharacterCodes();
        }

        #endregion

        #region Public Properties

        /// <summary>
        ///   Gets or sets State.
        /// </summary>
        public ObservableCollection<SubtitleTrack> SubtitleTracks
        {
            get
            {
                return this.subtitleTracks;
            }

            set
            {
                this.subtitleTracks = value;
                this.NotifyOfPropertyChange(() => this.SubtitleTracks);
            }
        }

        /// <summary>
        /// Gets or sets Langauges.
        /// </summary>
        public IEnumerable<string> Langauges { get; set; }

        /// <summary>
        /// Gets or sets CharacterCodes.
        /// </summary>
        public IEnumerable<string> CharacterCodes { get; set; }


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
                this.NotifyOfPropertyChange(() => SourceTracks);
            }
        }

        #endregion

        #region Public Methods

        /// <summary>
        /// Add a new Track
        /// </summary>
        public void Add()
        {
            if (this.SourceTracks != null)
            {
                Subtitle source = this.SourceTracks.FirstOrDefault();
                if (source != null)
                {
                    SubtitleTrack track = new SubtitleTrack
                    {
                        SubtitleType = SubtitleType.VobSub
                    };

                    this.SubtitleTracks.Add(track);
                }
            }
        }

        /// <summary>
        /// Import an SRT File.
        /// </summary>
        public void Import()
        {
            VistaOpenFileDialog dialog = new VistaOpenFileDialog { Filter = "SRT files (*.srt)|*.srt", CheckFileExists = true, Multiselect = true };

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
                this.SubtitleTracks.Add(track);
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
            this.SubtitleTracks.Remove(track);
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
            this.SubtitleTracks = task.SubtitleTracks;
        }

        #endregion
    }
}