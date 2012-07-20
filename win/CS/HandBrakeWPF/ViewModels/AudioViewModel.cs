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
    using System.Collections.Specialized;
    using System.ComponentModel.Composition;
    using System.Linq;

    using Caliburn.Micro;

    using HandBrake.ApplicationServices.Model;
    using HandBrake.ApplicationServices.Model.Encoding;
    using HandBrake.ApplicationServices.Parsing;
    using HandBrake.ApplicationServices.Services.Interfaces;
    using HandBrake.ApplicationServices.Utilities;
    using HandBrake.Interop.Model.Encoding;

    using HandBrakeWPF.ViewModels.Interfaces;

    /// <summary>
    /// The Audio View Model
    /// </summary>
    [Export(typeof(IAudioViewModel))]
    public class AudioViewModel : ViewModelBase, IAudioViewModel
    {
        /// <summary>
        /// Backing field for the source tracks list.
        /// </summary>
        private IEnumerable<Audio> sourceTracks;

        #region Constructors and Destructors

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
            this.Task = new EncodeTask();
            this.SampleRates = new ObservableCollection<string> { "Auto", "48", "44.1", "32", "24", "22.05" };
            this.AudioBitrates = this.GetAppropiateBitrates(AudioEncoder.ffaac, Mixdown.DolbyProLogicII);
            this.AudioEncoders = EnumHelper<AudioEncoder>.GetEnumList();
            this.AudioMixdowns = EnumHelper<Mixdown>.GetEnumList();
            this.SourceTracks = new List<Audio>();
        }

        #endregion

        #region Properties

        /// <summary>
        /// Gets or sets AudioBitrates.
        /// </summary>
        public IEnumerable<int> AudioBitrates { get; set; }

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
        /// Gets or sets SourceTracks.
        /// </summary>
        public IEnumerable<Audio> SourceTracks
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
        /// Gets or sets the EncodeTask.
        /// </summary>
        public EncodeTask Task { get; set; }

        /// <summary>
        /// Gets a value indicating whether ShowPassthruOptions.
        /// </summary>
        public bool ShowPassthruOptions
        {
            get
            {
                return this.UserSettingService.GetUserSetting<bool>(UserSettingConstants.ShowAdvancedAudioPassthruOpts);
            }
        }

        #endregion

        #region Public Methods

        /// <summary>
        /// Add an Audio Track
        /// </summary>
        public void Add()
        {
            // Add the first track if available.
            this.Add(null);
        }

        /// <summary>
        /// Remove the Selected Track
        /// </summary>
        /// <param name="track">
        /// The track.
        /// </param>
        public void Remove(AudioTrack track)
        {
            this.Task.AudioTracks.Remove(track);
        }

        /// <summary>
        /// Trigger a Notify Property Changed on the Task to force various UI elements to update.
        /// </summary>
        public void RefreshTask()
        {
            this.NotifyOfPropertyChange(() => this.Task);

            if (Task.OutputFormat == OutputFormat.Mp4)
            {
                foreach (AudioTrack track in this.Task.AudioTracks.Where(track => track.Encoder == AudioEncoder.ffflac || track.Encoder == AudioEncoder.Vorbis))
                {
                    track.Encoder = AudioEncoder.ffaac;
                }
            }
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

            if (preset != null && preset.Task != null)
            {
                this.AddTracksFromPreset(preset);
                this.Task.AllowedPassthruOptions = new AllowedPassthru(preset.Task.AllowedPassthruOptions);
            }
            this.NotifyOfPropertyChange(() => this.Task);

            this.Task.AllowedPassthruOptions.IsEnabled =
                 this.UserSettingService.GetUserSetting<bool>(UserSettingConstants.ShowAdvancedAudioPassthruOpts);
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
            this.NotifyOfPropertyChange(() => Task.AudioTracks);
            this.NotifyOfPropertyChange(() => this.Task);
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

            // Only reset the audio tracks if we have none, or if the task is null.
            if (this.Task == null || this.Task.AudioTracks.Count == 0)
            {
                this.SetPreset(preset, task);
            }

            // If there are no source tracks, clear the list, otherwise try to Auto-Select the correct tracks
            if (this.SourceTracks == null || !this.SourceTracks.Any())
            {
                this.Task.AudioTracks.Clear();
            }
            else
            {
                this.AutomaticTrackSelection();
            }

            // Force UI Updates
            this.NotifyOfPropertyChange(() => this.Task);
        }

        #endregion

        #endregion

        #region Methods

        /// <summary>
        /// Add the specified source track, or the first track in the SourceTracks collection if available.
        /// </summary>
        /// <param name="sourceTrack">
        /// The source track.
        /// </param>
        private void Add(Audio sourceTrack)
        {
            if (this.SourceTracks != null)
            {
                Audio track = sourceTrack ?? this.SourceTracks.FirstOrDefault();
                if (track != null)
                {
                    this.Task.AudioTracks.Add(new AudioTrack { ScannedTrack = track });
                }
            }
        }

        /// <summary>
        /// Add all source tracks that don't currently exist on the list.
        /// </summary>
        private void AddAllRemaining()
        {
            foreach (Audio sourceTrack in this.SourceTracks)
            {
                bool found = this.Task.AudioTracks.Any(audioTrack => audioTrack.ScannedTrack == sourceTrack);
                if (!found)
                {
                    this.Add(sourceTrack);
                }
            }
        }

        /// <summary>
        /// Add the required tracks for the current preset
        /// </summary>
        /// <param name="preset">
        /// The preset.
        /// </param>
        private void AddTracksFromPreset(Preset preset)
        {
            // Clear out the old tracks
            this.Task.AudioTracks.Clear();

            // Get the preferred Language
            IEnumerable<Audio> preferredAudioTracks =
                this.SourceTracks.Where(
                    item =>
                    item.Language.Contains(
                        this.UserSettingService.GetUserSetting<string>(UserSettingConstants.NativeLanguage)));
            Audio preferred = preferredAudioTracks.FirstOrDefault() ?? this.SourceTracks.FirstOrDefault();

            // Add the preset audio tracks with the preferred language
            foreach (AudioTrack track in preset.Task.AudioTracks)
            {
                this.Task.AudioTracks.Add(new AudioTrack(track) { ScannedTrack = preferred });
            }
        }

        /// <summary>
        /// Attempt to automatically select the correct audio tracks based on the users settings.
        /// </summary>
        private void AutomaticTrackSelection()
        {
            List<Audio> trackList = new List<Audio>();
            if (!this.SourceTracks.Any())
            {
                return;
            }

            // Step 1: Fetch all the Preferred Language settings
            if (this.UserSettingService.GetUserSetting<string>(UserSettingConstants.NativeLanguage) == "Any")
            {
                // If we have Any as the preferred Language, just set the first track to all audio tracks.
                trackList.Add(this.SourceTracks.FirstOrDefault());
            }
            else
            {
                // Otherwise, fetch the preferred language.
                foreach (
                    Audio item in
                        this.SourceTracks.Where(
                            item =>
                            item.Language.Contains(
                                this.UserSettingService.GetUserSetting<string>(UserSettingConstants.NativeLanguage))))
                {
                    trackList.Add(item);
                    break;
                }
            }

            // Step 2: Handle "All Remaining Tracks" and "All for Selected Languages" or use the default behaviour
            int mode = this.UserSettingService.GetUserSetting<int>(UserSettingConstants.DubModeAudio);

            switch (mode)
            {
                default: // Default by setting all existing tracks to a preferred or first source track.
                    foreach (AudioTrack track in this.Task.AudioTracks)
                    {
                        track.ScannedTrack = trackList.FirstOrDefault() ?? this.SourceTracks.FirstOrDefault();
                    }
                    break;
                case 1: // Adding all remaining audio tracks
                    this.AddAllRemaining();
                    break;
                case 2: // Add Langauges tracks for the additional languages selected, in-order.

                    // Figure out the source tracks we want to add
                    trackList.Clear();
                    foreach (
                        string language in
                            this.UserSettingService.GetUserSetting<StringCollection>(
                                UserSettingConstants.SelectedLanguages))
                    {
                        // TODO add support for "Add only 1 per language"
                        trackList.AddRange(this.SourceTracks.Where(source => source.Language.Trim() == language));
                    }

                    // Add them if they are not already added.
                    foreach (Audio sourceTrack in trackList)
                    {
                        bool found = this.Task.AudioTracks.Any(audioTrack => audioTrack.ScannedTrack == sourceTrack);

                        if (!found)
                        {
                            this.Add(sourceTrack);
                        }
                    }

                    break;
            }
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
            return new ObservableCollection<int> { 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384, 448, 640, 768 };
        }

        #endregion
    }
}