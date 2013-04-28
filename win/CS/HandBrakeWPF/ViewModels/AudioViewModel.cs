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
    using System;
    using System.Collections.Generic;
    using System.Collections.ObjectModel;
    using System.Collections.Specialized;
    using System.Linq;

    using Caliburn.Micro;

    using HandBrake.ApplicationServices.Model;
    using HandBrake.ApplicationServices.Model.Encoding;
    using HandBrake.ApplicationServices.Parsing;
    using HandBrake.ApplicationServices.Services.Interfaces;
    using HandBrake.ApplicationServices.Utilities;
    using HandBrake.Interop.Model.Encoding;

    using HandBrakeWPF.Commands;
    using HandBrakeWPF.Model;
    using HandBrakeWPF.ViewModels.Interfaces;

    /// <summary>
    /// The Audio View Model
    /// </summary>
    public class AudioViewModel : ViewModelBase, IAudioViewModel
    {
        /// <summary>
        /// Backing field for the source tracks list.
        /// </summary>
        private IEnumerable<Audio> sourceTracks;

        private Preset currentPreset;

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
            this.SampleRates = new ObservableCollection<string> { "Auto", "48", "44.1", "32", "24", "22.05", "16", "12", "11.025", "8" };
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
        /// The add all remaining.
        /// </summary>
        public void AddAllRemaining()
        {
            this.AddAllRemainingTracks();
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
        /// Clear out the Audio Tracks
        /// </summary>
        public void Clear()
        {
            this.Task.AudioTracks.Clear();
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
            this.Task = task;
            this.currentPreset = preset;

            if (preset != null && preset.Task != null)
            {
                this.AddTracksFromPreset(preset);
                this.Task.AllowedPassthruOptions = new AllowedPassthru(preset.Task.AllowedPassthruOptions);
            }
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
                Audio track = sourceTrack ?? this.GetPreferredAudioTrack();
                if (track != null)
                {
                    this.Task.AudioTracks.Add(new AudioTrack { ScannedTrack = track });
                }
            }
        }

        /// <summary>
        /// Add all source tracks that don't currently exist on the list.
        /// </summary>
        private void AddAllRemainingTracks()
        {
            // For all the source audio tracks
            foreach (Audio sourceTrack in this.SourceTracks)
            {
                // Step 1:  If "Add only One per language" is turned on, check to see if this language is already added.
                if (this.CanSkipSourceTrack(sourceTrack))
                {
                    continue;
                }

                // Step 2: Check if the track list already contrains this track
                bool found = this.Task.AudioTracks.Any(audioTrack => Equals(audioTrack.ScannedTrack, sourceTrack));
                if (!found)
                {
                    // If it doesn't, add it.
                    this.Add(sourceTrack);
                }
            }
        }

        /// <summary>
        /// Add all remaining for selected languages.
        /// </summary>
        public void AddAllRemainingForSelectedLanguages()
        {
            // Add them if they are not already added.
            foreach (Audio sourceTrack in this.GetSelectedLanguagesTracks())
            {
                // Step 1:  If "Add only One per language" is turned on, check to see if this language is already added.
                if (this.CanSkipSourceTrack(sourceTrack))
                {
                    continue;
                }

                // Step 2: Check if the track list already contrains this track
                bool found = this.Task.AudioTracks.Any(audioTrack => Equals(audioTrack.ScannedTrack, sourceTrack));
                if (!found)
                {
                    // If it doesn't, add it.
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

            // Add the preset audio tracks with the preferred language
            foreach (AudioTrack track in preset.Task.AudioTracks)
            {
                this.Task.AudioTracks.Add(new AudioTrack(track) { ScannedTrack = this.GetPreferredAudioTrack() });
            }
        }

        /// <summary>
        /// Attempt to automatically select the correct audio tracks based on the users settings.
        /// </summary>
        private void AutomaticTrackSelection()
        {
            if (!this.SourceTracks.Any())
            {
                // Clear out the old tracks
                this.Task.AudioTracks.Clear();

                return;
            }

            // We've changed source, so lets try reset the language, description and formats as close as possible to the previous track.
            foreach (AudioTrack track in this.Task.AudioTracks)
            {
                track.ScannedTrack = this.GetPreferredAudioTrack();
            }

            // Handle the default selection behaviour.
            int mode = this.UserSettingService.GetUserSetting<int>(UserSettingConstants.DubModeAudio);
            if (mode == 1 || mode == 2)
            {
                // First, we'll clear out all current tracks and go back to what the current preset has.
                // This will alteast provide a consistent behavior when switching tracks.
                this.Task.AudioTracks.Clear();
                this.AddTracksFromPreset(this.currentPreset);
            }

            switch (mode)
            {
                case 1: // Adding all remaining audio tracks
                    this.AddAllRemaining();
                    break;
                case 2: // Add Langauges tracks for the additional languages selected, in-order.
                    this.AddAllRemainingForSelectedLanguages();
                    break;
            }
        }

        /// <summary>
        /// The get preferred audio track, or the first if none available.
        /// </summary>
        /// <returns>
        /// The users preferred language, or the first if none available.
        /// </returns>
        private Audio GetPreferredAudioTrack()
        {
            // Get the preferred Language
            IEnumerable<Audio> preferredAudioTracks =
                this.SourceTracks.Where(
                    item =>
                    item.Language.Contains(
                        this.UserSettingService.GetUserSetting<string>(UserSettingConstants.NativeLanguage)));
            return preferredAudioTracks.FirstOrDefault() ?? this.SourceTracks.FirstOrDefault();
        }

        /// <summary>
        /// Gets a list of source tracks for the users selected languages.
        /// </summary>
        /// <returns>
        /// A list of source audio tracks.
        /// </returns>
        private IEnumerable<Audio> GetSelectedLanguagesTracks()
        {
            List<Audio> trackList = new List<Audio>();
            foreach (string language in this.UserSettingService.GetUserSetting<StringCollection>(UserSettingConstants.SelectedLanguages))
            {
                trackList.AddRange(this.SourceTracks.Where(source => source.Language.Trim() == language));
            }

            return trackList;
        }

        /// <summary>
        /// Checks to see if we can skip over the given source audio track.
        /// True when the user has set "Add only one per language" feature AND the language is contained in the track list.
        /// </summary>
        /// <param name="sourceTrack">
        /// The source track.
        /// </param>
        /// <returns>
        /// True when the user has set "Add only one per language" feature AND the language is contained in the track list
        /// </returns>
        private bool CanSkipSourceTrack(Audio sourceTrack)
        {
            bool addOnlyOnePerLanguage = this.UserSettingService.GetUserSetting<bool>(UserSettingConstants.AddOnlyOneAudioPerLanguage);
            bool sourceTrackLanguageFound = this.Task.AudioTracks.Any(audioTrack => audioTrack.ScannedTrack != null && sourceTrack.Language == audioTrack.ScannedTrack.Language);
            if (addOnlyOnePerLanguage && sourceTrackLanguageFound)
            {
                return true; // This track can be skipped.
            }

            return false;
        }

        #endregion
    }
}