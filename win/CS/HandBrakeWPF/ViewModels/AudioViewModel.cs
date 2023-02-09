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
    using System.Linq;

    using HandBrake.Interop.Interop;
    using HandBrake.Interop.Interop.Interfaces.Model.Encoders;
    using HandBrake.Interop.Utilities;

    using HandBrakeWPF.Commands;
    using HandBrakeWPF.EventArgs;
    using HandBrakeWPF.Model.Audio;
    using HandBrakeWPF.Services.Interfaces;
    using HandBrakeWPF.Services.Presets.Model;
    using HandBrakeWPF.Services.Scan.Model;
    using HandBrakeWPF.ViewModels.Interfaces;

    using AudioTrack = Services.Encode.Model.Models.AudioTrack;
    using EncodeTask = Services.Encode.Model.EncodeTask;
    using OutputFormat = Services.Encode.Model.Models.OutputFormat;

    /// <summary>
    /// The Audio View Model
    /// </summary>
    public class AudioViewModel : ViewModelBase, IAudioViewModel
    {
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
            this.AudioDefaultsViewModel = new AudioDefaultsViewModel(windowManager);

            this.SampleRates = new ObservableCollection<string> { "Auto" };
            foreach (var item in HandBrakeEncoderHelpers.AudioSampleRates)
            {
                this.SampleRates.Add(item.Name);
            }

            this.AudioEncoders = HandBrakeEncoderHelpers.AudioEncoders.ToList();
            this.SourceTracks = new List<Audio>();
            this.RemoveCommand = new SimpleRelayCommand<AudioTrack>(this.Remove);
        }

        public SimpleRelayCommand<AudioTrack> RemoveCommand { get; set; }

        #endregion

        public event EventHandler<TabStatusEventArgs> TabStatusChanged;

        #region Properties

        /// <summary>
        /// Gets or sets the audio defaults view model.
        /// </summary>
        public IAudioDefaultsViewModel AudioDefaultsViewModel { get; set; }

        public ListboxDeleteCommand DeleteCommand => new ListboxDeleteCommand();

        /// <summary>
        /// Gets or sets AudioEncoders.
        /// </summary>
        public IEnumerable<HBAudioEncoder> AudioEncoders { get; set; }

        /// <summary>
        /// Gets or sets SampleRates.
        /// </summary>
        public IList<string> SampleRates { get; set; }

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
        /// Gets the default audio behaviours. 
        /// </summary>
        public AudioBehaviours AudioBehaviours { get; private set; }

        #endregion

        #region Public Methods

        /// <summary>
        /// Add an Audio Track
        /// </summary>
        public void Add()
        {
            // Add the first track if available.
            this.Add(null, false);
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
        /// Reload the audio tracks based on the defaults.
        /// </summary>
        public void ReloadDefaults()
        {
            this.SetupTracks();
        }

        /// <summary>
        /// Trigger a Notify Property Changed on the Task to force various UI elements to update.
        /// </summary>
        public void RefreshTask(OutputFormat format)
        {
            if (format == OutputFormat.Mp4)
            {
                foreach (AudioTrack track in this.Task.AudioTracks)
                {
                    if (!track.Encoder.SupportsMP4)
                    {
                        track.Encoder = HandBrakeEncoderHelpers.GetAudioEncoder(HBAudioEncoder.AvAac);
                    }
                }
            }

            if (format == OutputFormat.WebM)
            {
                foreach (AudioTrack track in this.Task.AudioTracks)
                {
                    if (!track.Encoder.SupportsWebM)
                    {
                        track.Encoder = HandBrakeEncoderHelpers.GetAudioEncoder(HBAudioEncoder.Vorbis);
                    }
                }
            }

            this.NotifyOfPropertyChange(() => this.Task); // Trigger UI Refresh

            this.AudioDefaultsViewModel.RefreshTask(format);
        }

        /// <summary>
        /// The show audio defaults.
        /// </summary>
        public void ShowAudioDefaults()
        {
            this.AudioDefaultsViewModel.Setup(new AudioBehaviours(this.AudioBehaviours), this.Task.OutputFormat);

            if (this.AudioDefaultsViewModel.ShowWindow())
            {
                this.AudioBehaviours = new AudioBehaviours(this.AudioDefaultsViewModel.AudioBehaviours);
                this.Task.AudioPassthruOptions = this.AudioBehaviours.AllowedPassthruOptions;
                this.Task.AudioFallbackEncoder = this.AudioBehaviours.AudioFallbackEncoder;

                this.OnTabStatusChanged(null);
            }
        }

        public void ExpandAllTracks()
        {
            foreach (var track in this.Task.AudioTracks)
            {
                track.IsExpandedTrackView = true;
            }
        }

        public void CollapseAllTracks()
        {
            foreach (var track in this.Task.AudioTracks)
            {
                track.IsExpandedTrackView = false;
            }
        }

        #endregion

        #region Implemented Interfaces

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

            // Audio Behaviours
            this.AudioDefaultsViewModel.Setup(preset.AudioTrackBehaviours, preset.Task.OutputFormat);
            this.AudioBehaviours = new AudioBehaviours(preset.AudioTrackBehaviours);
            this.Task.AudioPassthruOptions = this.AudioBehaviours.AllowedPassthruOptions;
            this.Task.AudioFallbackEncoder = this.AudioBehaviours.AudioFallbackEncoder;

            if (preset.Task != null)
            {
                this.SetupTracks();
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
            this.NotifyOfPropertyChange(() => this.Task.AudioTracks);
            this.NotifyOfPropertyChange(() => this.Task);
        }

        public bool MatchesPreset(Preset preset)
        {
            // Check the default behaviours still match the preset.
            if (preset.AudioTrackBehaviours.SelectedBehaviour != this.AudioBehaviours.SelectedBehaviour)
            {
                return false;
            }

            if (preset.AudioTrackBehaviours.SelectedTrackDefaultBehaviour
                != this.AudioBehaviours.SelectedTrackDefaultBehaviour)
            {
                return false;
            }

            foreach (var item in this.AudioBehaviours.SelectedLanguages)
            {
                if (!preset.AudioTrackBehaviours.SelectedLanguages.Contains(item))
                {
                    return false;
                }
            }

            // Check if we have missing fallback options.
            foreach (HBAudioEncoder encoder in this.Task.AudioPassthruOptions)
            {
                if (!preset.AudioTrackBehaviours.AllowedPassthruOptions.Contains(encoder))
                {
                    return false;
                }
            }

            // Other direction
            foreach (HBAudioEncoder encoder in preset.AudioTrackBehaviours.AllowedPassthruOptions)
            {
                if (!this.Task.AudioPassthruOptions.Contains(encoder))
                {
                    return false;
                }
            }

            if (preset.AudioTrackBehaviours.AudioFallbackEncoder != this.Task.AudioFallbackEncoder)
            {
                return false;
            }

            foreach (var language in preset.AudioTrackBehaviours.SelectedLanguages)
            {
                if (!this.AudioBehaviours.SelectedLanguages.Contains(language))
                {
                    return false;
                }
            }

            if (preset.AudioTrackBehaviours.SelectedLanguages.Count != this.AudioBehaviours.SelectedLanguages.Count)
            {
                return false;
            }

            if (preset.AudioTrackBehaviours.BehaviourTracks.Count != this.AudioBehaviours.BehaviourTracks.Count)
            {
                return false;
            }

            return true;
        }

        /// <summary>
        /// Set the Source Title
        /// </summary>
        /// <param name="source">
        /// The source.
        /// </param>
        /// <param name="title">
        /// The title.
        /// </param>
        /// <param name="preset">
        /// The preset.
        /// </param>
        /// <param name="task">
        /// The task.
        /// </param>
        public void SetSource(Source source, Title title, Preset preset, EncodeTask task)
        {
            this.SourceTracks = title.AudioTracks;

            // Only reset the audio tracks if we have none, or if the task is null.
            if (this.Task == null)
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
                this.SetupTracks();
            }

            // Force UI Updates
            this.NotifyOfPropertyChange(() => this.Task);
        }

        #endregion

        #region Methods
        protected virtual void OnTabStatusChanged(TabStatusEventArgs e)
        {
            this.TabStatusChanged?.Invoke(this, e);
        }

        /// <summary>
        /// Add the specified source track, or the first track in the SourceTracks collection if available.
        /// </summary>
        /// <param name="sourceTrack">
        /// The source track.
        /// </param>
        /// <param name="useBehaviourTemplateMode">
        /// The use Behaviour Template Mode.
        /// </param>
        private void Add(Audio sourceTrack, bool useBehaviourTemplateMode)
        {
            if (this.SourceTracks != null)
            {
                Audio track = sourceTrack ?? this.GetPreferredAudioTrack();
                if (track != null)
                {
                    if (!useBehaviourTemplateMode)
                    {
                        this.Task.AudioTracks.Add(new AudioTrack { ScannedTrack = track });
                        return;
                    }

                    switch (this.AudioBehaviours.SelectedTrackDefaultBehaviour)
                    {
                        case AudioTrackDefaultsMode.FirstTrack:
                            AudioBehaviourTrack template = this.AudioBehaviours.BehaviourTracks.FirstOrDefault();
                            if (this.CanAddTrack(template, track, this.AudioBehaviours.AudioFallbackEncoder))
                            {
                                this.Task.AudioTracks.Add( template != null ? new AudioTrack(template, track, this.AudioBehaviours.AllowedPassthruOptions, this.AudioBehaviours.AudioFallbackEncoder, this.Task.OutputFormat) : new AudioTrack { ScannedTrack = track });
                            }
                            break;
                        case AudioTrackDefaultsMode.AllTracks:
                            foreach (AudioBehaviourTrack tmpl in this.AudioBehaviours.BehaviourTracks)
                            {
                                if (this.CanAddTrack(tmpl, track, this.AudioBehaviours.AudioFallbackEncoder))
                                {
                                    this.Task.AudioTracks.Add(tmpl != null ? new AudioTrack(tmpl, track, this.AudioBehaviours.AllowedPassthruOptions, this.AudioBehaviours.AudioFallbackEncoder, this.Task.OutputFormat) : new AudioTrack { ScannedTrack = track });
                                }
                            }

                            break;
                    }
                }
            }
        }

        private bool CanAddTrack(AudioBehaviourTrack track, Audio sourceTrack, HBAudioEncoder fallback)
        {
            if (fallback == HBAudioEncoder.None && track != null)
            {
                if (track.IsPassthru && (sourceTrack.Codec & track.Encoder.Id) == 0)
                {
                    return false;
                }
            }

            return true;
        }

        /// <summary>
        /// Attempt to automatically select the correct audio tracks based on the users settings.
        /// </summary>
        private void SetupTracks()
        {
            if (!this.SourceTracks.Any())
            {
                // Clear out the old tracks
                this.Task.AudioTracks.Clear();

                return;
            }

            // Step 1, Cleanup Previous Tracks
            this.Task.AudioTracks.Clear();

            // Step 2, Sanity Check
            if (this.SourceTracks == null || !this.SourceTracks.Any())
            {
                return;
            }

            // Step 3, Setup the tracks from the preset
            foreach (AudioBehaviourTrack track in this.AudioBehaviours.BehaviourTracks)
            {
                Audio sourceTrack = this.GetPreferredAudioTrack();
                if (this.CanAddTrack(track, sourceTrack, this.AudioBehaviours.AudioFallbackEncoder))
                {
                    this.Task.AudioTracks.Add(new AudioTrack(track, sourceTrack, this.AudioBehaviours.AllowedPassthruOptions, this.AudioBehaviours.AudioFallbackEncoder, this.Task.OutputFormat));
                }
            }
           
            // Step 4, Handle the default selection behaviour.
            switch (this.AudioBehaviours.SelectedBehaviour)
            {
                case AudioBehaviourModes.None:
                    this.Task.AudioTracks.Clear();
                    break;
                case AudioBehaviourModes.FirstMatch: // Adding all remaining audio tracks
                    this.AddFirstForSelectedLanguages();
                    break;
                case AudioBehaviourModes.AllMatching: // Add Languages tracks for the additional languages selected, in-order.
                    this.AddAllRemainingForSelectedLanguages();
                    break;
            }
        }

        /// <summary>
        /// The add first for selected languages.
        /// </summary>
        private void AddFirstForSelectedLanguages()
        {
            bool anyLanguageSelected = this.AudioBehaviours.SelectedLanguages.Contains(Constants.Any);

            if (anyLanguageSelected && this.Task.AudioTracks.Count >= 1)
            {
                return;
            }

            foreach (Audio sourceTrack in this.GetSelectedLanguagesTracks())
            {
                // Step 2: Check if the track list already contains this track
                bool found = this.Task.AudioTracks.Any(audioTrack => Equals(audioTrack.ScannedTrack, sourceTrack));
                if (!found)
                {
                    // Check if we are already using this language
                    bool foundLanguage = false;
                    foreach (var item in this.Task.AudioTracks.Where(item => item.ScannedTrack != null && sourceTrack.LanguageCode.Contains(item.ScannedTrack.LanguageCode)))
                    {
                        foundLanguage = true;
                    }

                    if (foundLanguage)
                    {
                        continue;
                    }

                    // If it doesn't, add it.
                    this.Add(sourceTrack, true);

                    // If we are using "(Any)" then break here. We only add the first track in this instance.
                    if (anyLanguageSelected)
                    {
                        break;
                    }
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
                // Step 2: Check if the track list already contains this track
                bool found = this.Task.AudioTracks.Any(audioTrack => Equals(audioTrack.ScannedTrack, sourceTrack));
                if (!found)
                {
                    // If it doesn't, add it.
                    this.Add(sourceTrack, true);
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
                // Step 2: Check if the track list already contains this track
                bool found = this.Task.AudioTracks.Any(audioTrack => Equals(audioTrack.ScannedTrack, sourceTrack));
                if (!found)
                {
                    // If it doesn't, add it.
                    this.Add(sourceTrack, true);
                }
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
            // The first track in the selected languages list is considered the preferred language.
            // So, try match tracks on this.
            IEnumerable<Audio> preferredAudioTracks = new List<Audio>();
            if (this.AudioBehaviours.SelectedLanguages.Count > 0)
            {
                string langName = this.AudioBehaviours.SelectedLanguages.FirstOrDefault(w => !w.Equals(Constants.Any));
                string langCode = LanguageUtilities.GetLanguageCode(langName);
                if (!string.IsNullOrEmpty(langCode))
                {
                    preferredAudioTracks = this.SourceTracks.Where(item => item.LanguageCode.Contains(langCode));
                }
            }

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
            // Translate to Iso Codes
            List<string> iso6392Codes = new List<string>();
            if (this.AudioBehaviours.SelectedLanguages.Contains(Constants.Any))
            {
                iso6392Codes = LanguageUtilities.GetIsoCodes();
                iso6392Codes = LanguageUtilities.OrderIsoCodes(iso6392Codes, this.AudioBehaviours.SelectedLanguages);
            }
            else
            {
                iso6392Codes = LanguageUtilities.GetLanguageCodes(this.AudioBehaviours.SelectedLanguages.ToArray());
            }
            
            List<Audio> orderedTracks = new List<Audio>();
            foreach (string code in iso6392Codes)
            {
                orderedTracks.AddRange(this.SourceTracks.Where(audio => audio.LanguageCode == code));
            }

            return orderedTracks;
        }

        #endregion
    }
}