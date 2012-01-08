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
    using System;
    using System.Collections.ObjectModel;
    using System.ComponentModel.Composition;

    using Caliburn.Micro;

    using HandBrake.ApplicationServices.Model.Encoding;
    using HandBrake.ApplicationServices.Services.Interfaces;

    using HandBrakeWPF.ViewModels.Interfaces;

    using HandBrake.ApplicationServices.Model;

    /// <summary>
    /// The Subtitles View Model
    /// </summary>
    [Export(typeof(ISubtitlesViewModel))]
    public class SubtitlesViewModel : ViewModelBase, ISubtitlesViewModel
    {
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
        }

        /// <summary>
        /// Gets or sets State.
        /// </summary>
        public ObservableCollection<SubtitleTrack> SubtitleTracks { get; set; }

        /// <summary>
        /// Set the currently selected preset.
        /// </summary>
        /// <param name="preset">
        /// The preset.
        /// </param>
        public void SetPreset(Preset preset)
        {  
        }

        /// <summary>
        /// Add a new Track
        /// </summary>
        public void Add()
        {
            this.SubtitleTracks.Add(new SubtitleTrack());
        }

        /// <summary>
        /// Remove a Track
        /// </summary>
        public void Remove()
        {
            throw new NotImplementedException();
        }

        /// <summary>
        /// Import an SRT File.
        /// </summary>
        public void Import()
        {
            throw new NotImplementedException();
        }
    }
}
