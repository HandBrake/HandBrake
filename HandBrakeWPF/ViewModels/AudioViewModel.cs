// --------------------------------------------------------------------------------------------------------------------
// <copyright file="AudioViewModel.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The Audio View Model
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.ViewModels
{
    using System.Collections.Generic;
    using System.Collections.ObjectModel;
    using System.ComponentModel.Composition;
    using System.Linq;

    using Caliburn.Micro;

    using HandBrake.ApplicationServices.Functions;
    using HandBrake.ApplicationServices.Model;
    using HandBrake.ApplicationServices.Model.Encoding;
    using HandBrake.ApplicationServices.Parsing;
    using HandBrake.ApplicationServices.Services.Interfaces;
    using HandBrake.Interop.Model.Encoding;

    using HandBrakeWPF.ViewModels.Interfaces;

    /// <summary>
    /// The Audio View Model
    /// </summary>
    [Export(typeof(IAudioViewModel))]
    public class AudioViewModel : ViewModelBase, IAudioViewModel
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="AudioViewModel"/> class.
        /// </summary>
        /// <param name="windowManager">
        /// The window manager.
        /// </param>
        /// <param name="userSettingService">
        /// The user Setting Service.
        /// </param>
        public AudioViewModel(IWindowManager windowManager, IUserSettingService userSettingService)
        {
            this.AudioTracks = new ObservableCollection<AudioTrack>();
            this.SampleRates = new ObservableCollection<string> { "Auto", "48", "44.1", "32", "24", "22.05" };
            this.AudioBitrates = this.GetAppropiateBitrates(AudioEncoder.ffaac, Mixdown.DolbyProLogicII);
            this.AudioEncoders = EnumHelper<AudioEncoder>.GetEnumList();
            this.AudioMixdowns = EnumHelper<Mixdown>.GetEnumList();
        }

        /// <summary>
        /// Gets or sets AudioTracks.
        /// </summary>
        public ObservableCollection<AudioTrack> AudioTracks { get; set; }

        /// <summary>
        /// Gets or sets SourceTracks.
        /// </summary>
        public IEnumerable<Audio> SourceTracks { get; set; }

        /// <summary>
        /// Gets or sets AudioEncoders.
        /// </summary>
        public IEnumerable<AudioEncoder> AudioEncoders { get; set; }

        /// <summary>
        /// Gets or sets AudioMixdowns.
        /// </summary>
        public IEnumerable<Mixdown> AudioMixdowns { get; set; }

        /// <summary>
        /// Gets or sets SampleRates.
        /// </summary>
        public IEnumerable<string> SampleRates { get; set; }

        /// <summary>
        /// Gets or sets AudioBitrates.
        /// </summary>
        public IEnumerable<int> AudioBitrates { get; set; }

        /// <summary>
        /// Add an Audio Track
        /// </summary>
        public void Add()
        {
            if (SourceTracks != null)
            {
                Audio track = this.SourceTracks.FirstOrDefault();
                if (track != null)
                {
                    this.AudioTracks.Add(new AudioTrack { ScannedTrack = track });
                }
            }
        }

        /// <summary>
        /// Remove the Selected Track
        /// </summary>
        /// <param name="track">
        /// The track.
        /// </param>
        public void Remove(AudioTrack track)
        {
            this.AudioTracks.Remove(track);
        }

        /// <summary>
        /// Set the Source Title
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
            this.SourceTracks = title.AudioTracks;
        }

        /// <summary>
        /// Get Appropiate Bitrates for the selected encoder and mixdown.
        /// </summary>
        /// <param name="encoder">
        /// The encoder.
        /// </param>
        /// <param name="mixdown">
        /// The mixdown.
        /// </param>
        /// <returns>
        /// A List of valid audio bitrates
        /// </returns>
        private IEnumerable<int> GetAppropiateBitrates(AudioEncoder encoder, Mixdown mixdown)
        {
            return new ObservableCollection<int> { 32, 40, 48, 56, 64, 80, 96, 112, 128, 160 };
        }
    }
}
