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
    using System.ComponentModel;
    using System.Linq;

    using Caliburn.Micro;

    using HandBrake.Interop.Interop;
    using HandBrake.Interop.Interop.Model.Encoding;
    using HandBrake.Interop.Utilities;

    using HandBrakeWPF.EventArgs;
    using HandBrakeWPF.Model.Audio;
    using HandBrakeWPF.Properties;
    using HandBrakeWPF.Services.Interfaces;
    using HandBrakeWPF.Services.Presets.Model;
    using HandBrakeWPF.Services.Scan.Model;
    using HandBrakeWPF.Utilities;
    using HandBrakeWPF.ViewModels.Interfaces;
    using HandBrakeWPF.Views;

    using AudioEncoder = HandBrakeWPF.Services.Encode.Model.Models.AudioEncoder;
    using AudioTrack = HandBrakeWPF.Services.Encode.Model.Models.AudioTrack;
    using EncodeTask = HandBrakeWPF.Services.Encode.Model.EncodeTask;
    using OutputFormat = HandBrakeWPF.Services.Encode.Model.Models.OutputFormat;

    /// <summary>
    /// The Audio View Model
    /// </summary>
    public class AudioViewModel : ViewModelBase, IAudioViewModel
    {
        private readonly IWindowManager windowManager;
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
            this.windowManager = windowManager;
            this.Task = new EncodeTask();
            this.AudioDefaultsViewModel = new AudioDefaultsViewModel(this.Task);

            this.SampleRates = new ObservableCollection<string> { "Auto" };
            foreach (var item in HandBrakeEncoderHelpers.AudioSampleRates)
            {
                this.SampleRates.Add(item.Name);
            }

            this.AudioEncoders = EnumHelper<AudioEncoder>.GetEnumList();
            this.SourceTracks = new List<Audio>();
        }

        #endregion

        public event EventHandler<TabStatusEventArgs> TabStatusChanged;

        #region Properties

        /// <summary>
        /// Gets or sets the audio defaults view model.
        /// </summary>
        public IAudioDefaultsViewModel AudioDefaultsViewModel { get; set; }

        /// <summary>
        /// Gets or sets AudioEncoders.
        /// </summary>
        public IEnumerable<AudioEncoder> AudioEncoders { get; set; }

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
        /// Gets the panel title.
        /// </summary>
        public string PanelTitle
        {
            get
            {
                return Resources.AudioViewModel_AudioTracks;
            }
        }

        /// <summary>
        /// Gets the switch display title.
        /// </summary>
        public string SwitchDisplayTitle
        {
            get
            {
                return Resources.AudioViewModel_ConfigureDefaults;
            }
        }

        /// <summary>
        /// Gets the default audio behaviours. 
        /// </summary>
        public AudioBehaviours AudioBehaviours
        {
            get
            {
                return this.AudioDefaultsViewModel.AudioBehaviours;
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
        public void RefreshTask()
        {
            this.NotifyOfPropertyChange(() => this.Task);

            if (this.Task.OutputFormat == OutputFormat.Mp4)
            {
                foreach (AudioTrack track in this.Task.AudioTracks.Where(track => track.Encoder == AudioEncoder.ffflac || track.Encoder == AudioEncoder.Vorbis))
                {
                    track.Encoder = AudioEncoder.ffaac;
                }
            }

            if (this.Task.OutputFormat == OutputFormat.WebM)
            {
                foreach (AudioTrack track in this.Task.AudioTracks.Where(track => track.Encoder != AudioEncoder.Vorbis && track.Encoder != AudioEncoder.Opus))
                {
                    track.Encoder = AudioEncoder.Vorbis;
                }
            }

            this.AudioDefaultsViewModel.RefreshTask();
        }

        /// <summary>
        /// The show audio defaults.
        /// </summary>
        public void ShowAudioDefaults()
        {
            if (this.windowManager.ShowDialog(this.AudioDefaultsViewModel) == true)
            {
                this.OnTabStatusChanged(null);
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
            this.currentPreset = preset;

            // Audio Behaviours
            this.AudioDefaultsViewModel.Setup(preset, task);

            if (preset != null && preset.Task != null)
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

            foreach (var item in this.AudioBehaviours.SelectedLangauges)
            {
                if (!preset.AudioTrackBehaviours.SelectedLangauges.Contains(item))
                {
                    return false;
                }
            }

            if (preset.Task.AllowedPassthruOptions.AudioAllowMP3Pass != this.Task.AllowedPassthruOptions.AudioAllowMP3Pass)
            {
                return false;
            }

            if (preset.Task.AllowedPassthruOptions.AudioAllowAACPass != this.Task.AllowedPassthruOptions.AudioAllowAACPass)
            {
                return false;
            }

            if (preset.Task.AllowedPassthruOptions.AudioAllowAC3Pass != this.Task.AllowedPassthruOptions.AudioAllowAC3Pass)
            {
                return false;
            }

            if (preset.Task.AllowedPassthruOptions.AudioAllowEAC3Pass != this.Task.AllowedPassthruOptions.AudioAllowEAC3Pass)
            {
                return false;
            }

            if (preset.Task.AllowedPassthruOptions.AudioAllowDTSPass != this.Task.AllowedPassthruOptions.AudioAllowDTSPass)
            {
                return false;
            }

            if (preset.Task.AllowedPassthruOptions.AudioAllowDTSHDPass != this.Task.AllowedPassthruOptions.AudioAllowDTSHDPass)
            {
                return false;
            }

            if (preset.Task.AllowedPassthruOptions.AudioAllowTrueHDPass != this.Task.AllowedPassthruOptions.AudioAllowTrueHDPass)
            {
                return false;
            }

            if (preset.Task.AllowedPassthruOptions.AudioAllowFlacPass != this.Task.AllowedPassthruOptions.AudioAllowFlacPass)
            {
                return false;
            }

            if (preset.Task.AllowedPassthruOptions.AudioEncoderFallback != this.Task.AllowedPassthruOptions.AudioEncoderFallback)
            {
                return false;
            }

            foreach (var language in preset.AudioTrackBehaviours.SelectedLangauges)
            {
                if (!this.AudioBehaviours.SelectedLangauges.Contains(language))
                {
                    return false;
                }
            }

            if (preset.AudioTrackBehaviours.SelectedLangauges.Count != this.AudioBehaviours.SelectedLangauges.Count)
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
                            if (this.CanAddTrack(template, track, this.Task.AllowedPassthruOptions.AudioEncoderFallback))
                            {
                                this.Task.AudioTracks.Add( template != null ? new AudioTrack(template, track, this.Task.AllowedPassthruOptions.AudioEncoderFallback) : new AudioTrack { ScannedTrack = track });
                            }
                            break;
                        case AudioTrackDefaultsMode.AllTracks:
                            foreach (AudioBehaviourTrack tmpl in this.AudioBehaviours.BehaviourTracks)
                            {
                                if (this.CanAddTrack(tmpl, track, this.Task.AllowedPassthruOptions.AudioEncoderFallback))
                                {
                                    this.Task.AudioTracks.Add(tmpl != null ? new AudioTrack(tmpl, track, this.Task.AllowedPassthruOptions.AudioEncoderFallback) : new AudioTrack { ScannedTrack = track });
                                }
                            }

                            break;
                    }
                }
            }
        }

        private bool CanAddTrack(AudioBehaviourTrack track, Audio sourceTrack, AudioEncoder fallback)
        {
            if (fallback == AudioEncoder.None && track != null)
            {
                HBAudioEncoder encoderInfo = HandBrakeEncoderHelpers.GetAudioEncoder(EnumHelper<AudioEncoder>.GetShortName(track.Encoder));
                if (track.IsPassthru && (sourceTrack.Codec & encoderInfo.Id) == 0)
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
                if (this.CanAddTrack(track, sourceTrack, this.Task.AllowedPassthruOptions.AudioEncoderFallback))
                {
                    this.Task.AudioTracks.Add(new AudioTrack(track, sourceTrack, this.Task.AllowedPassthruOptions.AudioEncoderFallback));
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
                case AudioBehaviourModes.AllMatching: // Add Langauges tracks for the additional languages selected, in-order.
                    this.AddAllRemainingForSelectedLanguages();
                    break;
            }
        }

        /// <summary>
        /// The add first for selected languages.
        /// </summary>
        private void AddFirstForSelectedLanguages()
        {
            bool anyLanguageSelected = this.AudioBehaviours.SelectedLangauges.Contains(Constants.Any);

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
                // Step 2: Check if the track list already contrains this track
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
                // Step 2: Check if the track list already contrains this track
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
            if (this.AudioBehaviours.SelectedLangauges.Count > 0)
            {
                string langName = this.AudioBehaviours.SelectedLangauges.FirstOrDefault(w => !w.Equals(Constants.Any));
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
            List<string> iso6392Codes = this.AudioBehaviours.SelectedLangauges.Contains(Constants.Any)
                ? LanguageUtilities.GetIsoCodes()
                : LanguageUtilities.GetLanguageCodes(this.AudioBehaviours.SelectedLangauges.ToArray());


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