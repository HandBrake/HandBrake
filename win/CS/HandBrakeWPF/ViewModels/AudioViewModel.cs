﻿// --------------------------------------------------------------------------------------------------------------------
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
    using System.ComponentModel;
    using System.Linq;

    using Caliburn.Micro;

    using HandBrake.ApplicationServices.Interop;
    using HandBrake.ApplicationServices.Interop.Model.Encoding;
    using HandBrake.ApplicationServices.Utilities;

    using HandBrakeWPF.Model.Audio;
    using HandBrakeWPF.Properties;
    using HandBrakeWPF.Services.Interfaces;
    using HandBrakeWPF.Services.Presets.Model;
    using HandBrakeWPF.Services.Scan.Model;
    using HandBrakeWPF.Utilities;
    using HandBrakeWPF.ViewModels.Interfaces;

    using AllowedPassthru = HandBrakeWPF.Services.Encode.Model.Models.AllowedPassthru;
    using AudioEncoder = HandBrakeWPF.Services.Encode.Model.Models.AudioEncoder;
    using AudioTrack = HandBrakeWPF.Services.Encode.Model.Models.AudioTrack;
    using EncodeTask = HandBrakeWPF.Services.Encode.Model.EncodeTask;
    using OutputFormat = HandBrakeWPF.Services.Encode.Model.Models.OutputFormat;

    /// <summary>
    /// The Audio View Model
    /// </summary>
    public class AudioViewModel : ViewModelBase, IAudioViewModel
    {
        /// <summary>
        /// Backing field for the source tracks list.
        /// </summary>
        private IEnumerable<Audio> sourceTracks;

        /// <summary>
        /// The current preset.
        /// </summary>
        private Preset currentPreset;

        /// <summary>
        /// The show audio defaults panel.
        /// </summary>
        private bool showAudioDefaultsPanel;

        /// <summary>
        /// The available languages.
        /// </summary>
        private BindingList<string> availableLanguages;

        /// <summary>
        /// The audio behaviours.
        /// </summary>
        private AudioBehaviours audioBehaviours;

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

            this.SampleRates = new ObservableCollection<string> { "Auto" };
            foreach (var item in HandBrakeEncoderHelpers.AudioSampleRates)
            {
                this.SampleRates.Add(item.Name);
            }

            this.AudioEncoders = EnumHelper<AudioEncoder>.GetEnumList();
            this.AudioMixdowns = EnumHelper<Mixdown>.GetEnumList();
            this.SourceTracks = new List<Audio>();

            this.AudioBehaviours = new AudioBehaviours();
            this.SelectedAvailableToMove = new BindingList<string>();
            this.SelectedLangaugesToMove = new BindingList<string>();
            this.AvailableLanguages = new BindingList<string>();
            this.SetupLanguages(null);
        }

        #endregion

        #region Properties

        /// <summary>
        /// Gets or sets the audio behaviours.
        /// </summary>
        public AudioBehaviours AudioBehaviours
        {
            get
            {
                return this.audioBehaviours;
            }
            set
            {
                if (Equals(value, this.audioBehaviours))
                {
                    return;
                }
                this.audioBehaviours = value;
                this.NotifyOfPropertyChange(() => this.AudioBehaviours);
            }
        }

        /// <summary>
        /// Gets the audio behaviour modes.
        /// </summary>
        public BindingList<AudioBehaviourModes> AudioBehaviourModeList
        {
            get
            {
                return new BindingList<AudioBehaviourModes>(EnumHelper<AudioBehaviourModes>.GetEnumList().ToList());
            }
        }

        /// <summary>
        /// Gets the audio track default behaviour mode list.
        /// </summary>
        public BindingList<AudioTrackDefaultsMode> AudioTrackDefaultBehaviourModeList
        {
            get
            {
                return new BindingList<AudioTrackDefaultsMode>(EnumHelper<AudioTrackDefaultsMode>.GetEnumList().ToList());
            }
        }

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
        /// Gets or sets a value indicating whether show audio defaults panel.
        /// </summary>
        public bool ShowAudioDefaultsPanel
        {
            get
            {
                return this.showAudioDefaultsPanel;
            }
            set
            {
                if (value.Equals(this.showAudioDefaultsPanel))
                {
                    return;
                }
                this.showAudioDefaultsPanel = value;
                this.NotifyOfPropertyChange(() => this.ShowAudioDefaultsPanel);
                this.NotifyOfPropertyChange(() => this.PanelTitle);
                this.NotifyOfPropertyChange(() => this.SwitchDisplayTitle);
            }
        }

        /// <summary>
        /// Gets the panel title.
        /// </summary>
        public string PanelTitle
        {
            get
            {
                return this.ShowAudioDefaultsPanel ? Resources.AudioViewModel_AudioDefaults : Resources.AudioViewModel_AudioTracks;
            }
        }

        /// <summary>
        /// Gets the switch display title.
        /// </summary>
        public string SwitchDisplayTitle
        {
            get
            {
                return this.ShowAudioDefaultsPanel ? Resources.AudioViewModel_SwitchBackToTracks : Resources.AudioViewModel_ConfigureDefaults;
            }
        }

        /// <summary>
        /// Gets or sets AvailableLanguages.
        /// </summary>
        public BindingList<string> AvailableLanguages
        {
            get
            {
                return this.availableLanguages;
            }

            set
            {
                this.availableLanguages = value;
                this.NotifyOfPropertyChange("AvailableLanguages");
            }
        }

        /// <summary>
        /// Gets or sets SelectedLangauges.
        /// </summary>
        public BindingList<string> SelectedAvailableToMove { get; set; }

        /// <summary>
        /// Gets or sets SelectedLangauges.
        /// </summary>
        public BindingList<string> SelectedLangaugesToMove { get; set; }

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
        }

        /// <summary>
        /// Open the options screen to the Audio and Subtitles tab.
        /// </summary>
        public void SetDefaultBehaviour()
        {
            this.ShowAudioDefaultsPanel = true;
        }

        /// <summary>
        /// The show audio defaults.
        /// </summary>
        public void ShowAudioDefaults()
        {
            // OpenOverlayPanelCommand command = new OpenOverlayPanelCommand();
            // command.Execute(new AudioDefaultsViewModel(this.WindowManager, this.UserSettingService));
            this.ShowAudioDefaultsPanel = !this.ShowAudioDefaultsPanel;
        }

        /// <summary>
        /// Audio List Move Left
        /// </summary>
        public void LanguageMoveRight()
        {
            if (this.SelectedAvailableToMove.Count > 0)
            {
                List<string> copiedList = this.SelectedAvailableToMove.ToList();
                foreach (string item in copiedList)
                {
                    this.AvailableLanguages.Remove(item);
                    this.AudioBehaviours.SelectedLangauges.Add(item);
                }

                this.AvailableLanguages = new BindingList<string>(this.AvailableLanguages.OrderBy(o => o).ToList());
            }
        }

        /// <summary>
        /// Audio List Move Right
        /// </summary>
        public void LanguageMoveLeft()
        {
            if (this.SelectedLangaugesToMove.Count > 0)
            {
                List<string> copiedList = this.SelectedLangaugesToMove.ToList();
                foreach (string item in copiedList)
                {
                    this.AudioBehaviours.SelectedLangauges.Remove(item);
                    this.AvailableLanguages.Add(item);
                }
            }

            this.AvailableLanguages = new BindingList<string>(this.AvailableLanguages.OrderBy(o => o).ToList());
        }

        /// <summary>
        /// Audio List Clear all selected languages
        /// </summary>
        public void LanguageClearAll()
        {
            foreach (string item in this.AudioBehaviours.SelectedLangauges)
            {
                this.AvailableLanguages.Add(item);
            }
            this.AvailableLanguages = new BindingList<string>(this.AvailableLanguages.OrderBy(o => o).ToList());

            this.AudioBehaviours.SelectedLangauges.Clear();
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
            this.SetupLanguages(preset);

            if (preset != null && preset.Task != null)
            {
                this.Task.AllowedPassthruOptions = new AllowedPassthru(preset.Task.AllowedPassthruOptions);

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
                        case AudioTrackDefaultsMode.None:
                            this.Task.AudioTracks.Add(new AudioTrack { ScannedTrack = track });
                            break;
                        case AudioTrackDefaultsMode.FirstTrack:
                            AudioTrack template = this.currentPreset.Task.AudioTracks.FirstOrDefault();
                            this.Task.AudioTracks.Add(template != null ? new AudioTrack(template, false) { ScannedTrack = track } : new AudioTrack { ScannedTrack = track });
                            break;
                        case AudioTrackDefaultsMode.AllTracks:
                            foreach (AudioTrack tmpl in this.currentPreset.Task.AudioTracks)
                            {
                                this.Task.AudioTracks.Add(tmpl != null ? new AudioTrack(tmpl, false) { ScannedTrack = track } : new AudioTrack { ScannedTrack = track });
                            }

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
            foreach (AudioTrack track in this.currentPreset.Task.AudioTracks)
            {
                this.Task.AudioTracks.Add(new AudioTrack(track, false) { ScannedTrack = this.GetPreferredAudioTrack() });
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
            foreach (Audio sourceTrack in this.GetSelectedLanguagesTracks(false))
            {
                // Step 2: Check if the track list already contrains this track
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
                }
            }
        }

        /// <summary>
        /// Add all remaining for selected languages.
        /// </summary>
        public void AddAllRemainingForSelectedLanguages()
        {
            // Add them if they are not already added.
            foreach (Audio sourceTrack in this.GetSelectedLanguagesTracks(true))
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
                if (!string.IsNullOrEmpty(langName))
                {
                    preferredAudioTracks = this.SourceTracks.Where(item => item.Language.Contains(langName));
                }
            }

            return preferredAudioTracks.FirstOrDefault() ?? this.SourceTracks.FirstOrDefault();
        }

        /// <summary>
        /// Gets a list of source tracks for the users selected languages.
        /// </summary>
        /// <param name="includeAny">
        /// The include Any.
        /// </param>
        /// <returns>
        /// A list of source audio tracks.
        /// </returns>
        private IEnumerable<Audio> GetSelectedLanguagesTracks(bool includeAny)
        {
            List<Audio> trackList = new List<Audio>();

            List<string> isoCodes = LanguageUtilities.GetLanguageCodes(this.AudioBehaviours.SelectedLangauges.ToArray());

            if (includeAny)
            {
                isoCodes = LanguageUtilities.GetIsoCodes();
            }

            foreach (string code in isoCodes)
            {
                trackList.AddRange(this.SourceTracks.Where(source => source.LanguageCode.Trim() == code));
            }

            return trackList;
        }

        /// <summary>
        /// The setup languages.
        /// </summary>
        /// <param name="preset">
        /// The preset.
        /// </param>
        private void SetupLanguages(Preset preset)
        {
            // Step 1, Set the behaviour mode
            this.AudioBehaviours.SelectedBehaviour = AudioBehaviourModes.None;
            this.AudioBehaviours.SelectedLangauges.Clear();

            // Step 2, Get all the languages
            IDictionary<string, string> langList = LanguageUtilities.MapLanguages();
            langList = (from entry in langList orderby entry.Key ascending select entry).ToDictionary(pair => pair.Key, pair => pair.Value);

            // Step 3, Setup Available Languages
            this.AvailableLanguages.Clear();
            foreach (string item in langList.Keys)
            {
                this.AvailableLanguages.Add(item);
            }

            // Step 4, Set the Selected Languages          
            if (preset != null && preset.AudioTrackBehaviours != null)
            {
                this.AudioBehaviours.SelectedBehaviour = preset.AudioTrackBehaviours.SelectedBehaviour;
                this.AudioBehaviours.SelectedTrackDefaultBehaviour = preset.AudioTrackBehaviours.SelectedTrackDefaultBehaviour;

                foreach (string selectedItem in preset.AudioTrackBehaviours.SelectedLangauges)
                {
                    this.AvailableLanguages.Remove(selectedItem);
                    this.AudioBehaviours.SelectedLangauges.Add(selectedItem);
                }
            }
        }

        #endregion
    }
}