// --------------------------------------------------------------------------------------------------------------------
// <copyright file="AudioDefaultsViewModel.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The Audio Defaults View Model
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.ViewModels
{
    using System.Collections.Generic;
    using System.Collections.ObjectModel;
    using System.ComponentModel;
    using System.Diagnostics;
    using System.Linq;

    using HandBrake.App.Core.Utilities;
    using HandBrake.Interop.Interop;
    using HandBrake.Interop.Interop.Interfaces.Model.Encoders;
    using HandBrake.Interop.Utilities;

    using HandBrakeWPF.Commands;
    using HandBrakeWPF.Model.Audio;
    using HandBrakeWPF.Properties;
    using HandBrakeWPF.Services.Encode.Model.Models;
    using HandBrakeWPF.Services.Interfaces;
    using HandBrakeWPF.ViewModels.Interfaces;
    using HandBrakeWPF.Views;

    /// <summary>
    /// The Audio View Model
    /// </summary>
    /// <remarks>
    /// TODO:
    /// - Support setting fallback encoder options for Passthru tracks.
    /// - Mixdown Dropdown should only show mixdowns for the set encoder. Not all.
    /// </remarks>
    public class AudioDefaultsViewModel : ViewModelBase, IAudioDefaultsViewModel
    {
        private readonly IWindowManager windowManager;

        private BindingList<string> availableLanguages;
        private AudioBehaviours audioBehaviours;

        /// <summary>
        /// Initializes a new instance of the <see cref="AudioDefaultsViewModel"/> class.
        /// </summary>
        public AudioDefaultsViewModel(IWindowManager windowManager)
        {
            this.windowManager = windowManager;
            this.AudioBehaviours = new AudioBehaviours();
            this.SelectedAvailableToMove = new BindingList<string>();
            this.SelectedLanguagesToMove = new BindingList<string>();
            this.AvailableLanguages = new BindingList<string>();
            this.AudioEncoders = new List<HBAudioEncoder>(HandBrakeEncoderHelpers.AudioEncoders) { HBAudioEncoder.None }; 

            this.SampleRates = new ObservableCollection<string> { "Auto" };
            foreach (var item in HandBrakeEncoderHelpers.AudioSampleRates)
            {
                this.SampleRates.Add(item.Name);
            }

            this.Title = Resources.AudioViewModel_AudioDefaults;

            BindingList<AudioFallbackWrapper> data = new BindingList<AudioFallbackWrapper>();
            foreach (HBAudioEncoder encoder in HandBrakeEncoderHelpers.AudioEncoders.Where(s => s.IsPassthrough && !s.IsAutoPassthru))
            {
                data.Add(new AudioFallbackWrapper(encoder));
            }

            this.PassthruEncoders = data;

            this.RemoveTrackCommand = new SimpleRelayCommand<AudioBehaviourTrack>(this.RemoveTrack, null);
        }

        #region Properties

        public ListboxDeleteCommand DeleteCommand => new ListboxDeleteCommand();

        public OutputFormat OutputFormat { get; private set; }

        /// <summary>
        /// Gets or sets the list of audio tracks we will use as templates for generating tracks for a given source.
        /// </summary>
        public BindingList<AudioBehaviourTrack> BehaviourTracks
        {
            get
            {
                return this.AudioBehaviours.BehaviourTracks;
            }
            set
            {
                this.AudioBehaviours.BehaviourTracks = value;
                this.NotifyOfPropertyChange(() => this.BehaviourTracks);
            }
        }

        /// <summary>
        /// Gets the audio behaviours.
        /// </summary>
        public AudioBehaviours AudioBehaviours
        {
            get
            {
                return this.audioBehaviours;
            }

            private set
            {
                if (Equals(value, this.audioBehaviours))
                {
                    return;
                }
                this.audioBehaviours = value;
                this.NotifyOfPropertyChange(() => this.AudioBehaviours);
            }
        }

        public bool IsApplied { get; set; }

        /// <summary>
        /// Gets SelectedLanguages.
        /// </summary>
        public BindingList<string> SelectedAvailableToMove { get; private set; }

        /// <summary>
        /// Gets SelectedLanguages.
        /// </summary>
        public BindingList<string> SelectedLanguagesToMove { get; private set; }

        public BindingList<AudioFallbackWrapper> PassthruEncoders { get; private set; }

        /// <summary>
        /// Gets or sets the audio encoder fallback.
        /// </summary>
        public HBAudioEncoder AudioEncoderFallback
        {
            get
            {
                return this.audioBehaviours.AudioFallbackEncoder;
            }

            set
            {
                if (value == this.audioBehaviours.AudioFallbackEncoder)
                {
                    return;
                }

                foreach (var item in this.BehaviourTracks)
                {
                    item.SetFallbackEncoder(value);
                }

                this.audioBehaviours.AudioFallbackEncoder = value;
                this.NotifyOfPropertyChange(() => this.AudioEncoderFallback);
            }
        }

        public SimpleRelayCommand<AudioBehaviourTrack> RemoveTrackCommand { get; }

        #endregion

        #region Data Properties 

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
        /// Gets AvailableLanguages.
        /// </summary>
        public BindingList<string> AvailableLanguages
        {
            get
            {
                return this.availableLanguages;
            }

            private set
            {
                this.availableLanguages = value;
                this.NotifyOfPropertyChange(() => this.AvailableLanguages);
            }
        }

        /// <summary>
        /// Gets or sets AudioEncoders.
        /// </summary>
        public IEnumerable<HBAudioEncoder> AudioEncoders { get; set; }

        /// <summary>
        /// Gets or sets SampleRates.
        /// </summary>
        public IList<string> SampleRates { get; set; }

        #endregion

        #region Public Methods

        /// <summary>
        /// Add a new behaviour track.
        /// </summary>
        public void AddTrack()
        {
            this.BehaviourTracks.Add(new AudioBehaviourTrack(this.AudioEncoderFallback));
            foreach (var item in this.BehaviourTracks)
            {
                item.SetFallbackEncoder(this.AudioEncoderFallback);
            }
        }

        /// <summary>
        /// Clear all the behaviour tracks
        /// </summary>
        public void ClearTracks()
        {
            this.BehaviourTracks.Clear();
        }

        /// <summary>
        /// Remove the Selected Track
        /// </summary>
        /// <param name="track">
        /// The track.
        /// </param>
        public void RemoveTrack(AudioBehaviourTrack track)
        {
            this.BehaviourTracks.Remove(track);
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
                    this.AudioBehaviours.SelectedLanguages.Add(item);
                }

                this.AvailableLanguages = new BindingList<string>(this.AvailableLanguages.OrderBy(o => o).ToList());
            }
        }

        /// <summary>
        /// Audio List Move Right
        /// </summary>
        public void LanguageMoveLeft()
        {
            if (this.SelectedLanguagesToMove.Count > 0)
            {
                List<string> copiedList = this.SelectedLanguagesToMove.ToList();
                foreach (string item in copiedList)
                {
                    this.AudioBehaviours.SelectedLanguages.Remove(item);
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
            foreach (string item in this.AudioBehaviours.SelectedLanguages)
            {
                this.AvailableLanguages.Add(item);
            }
            this.AvailableLanguages = new BindingList<string>(this.AvailableLanguages.OrderBy(o => o).ToList());

            this.AudioBehaviours.SelectedLanguages.Clear();
        }

        #endregion

        #region Methods

        public void Setup(AudioBehaviours behaviours, OutputFormat outputFormat)
        {
            // Reset
            this.IsApplied = false;
            this.AudioBehaviours = new AudioBehaviours();

            IDictionary<string, string> langList = LanguageUtilities.MapLanguages();
            langList = (from entry in langList orderby entry.Key ascending select entry).ToDictionary(pair => pair.Key, pair => pair.Value);

            this.AvailableLanguages.Clear();
            foreach (string item in langList.Keys)
            {
                this.AvailableLanguages.Add(item);
            }

            this.OutputFormat = outputFormat;

            if (behaviours != null)
            {
                this.AudioBehaviours.SelectedBehaviour = behaviours.SelectedBehaviour;
                this.AudioBehaviours.SelectedTrackDefaultBehaviour = behaviours.SelectedTrackDefaultBehaviour;
                this.AudioBehaviours.AllowedPassthruOptions = behaviours.AllowedPassthruOptions;
                this.AudioBehaviours.AudioFallbackEncoder = behaviours.AudioFallbackEncoder;

                foreach (var encoder in this.PassthruEncoders)
                {
                    encoder.IsEnabled = false;
                    if (behaviours.AllowedPassthruOptions.Contains(encoder.Encoder))
                    {
                        encoder.IsEnabled = true;
                    }
                }
                
                foreach (AudioBehaviourTrack item in behaviours.BehaviourTracks)
                {
                    this.BehaviourTracks.Add(new AudioBehaviourTrack(item));
                }

                this.NotifyOfPropertyChange(() => this.BehaviourTracks);

                foreach (string selectedItem in behaviours.SelectedLanguages)
                {
                    this.AvailableLanguages.Remove(selectedItem);
                    this.AudioBehaviours.SelectedLanguages.Add(selectedItem);
                }
            }

            this.CorrectAudioEncoders(this.OutputFormat);

            this.NotifyOfPropertyChange(() => this.PassthruEncoders);
            this.NotifyOfPropertyChange(() => this.AudioEncoderFallback);
        }

        /// <summary>
        /// The refresh task.
        /// </summary>
        public void RefreshTask(OutputFormat outputFormat)
        {
            this.OutputFormat = outputFormat;
            this.NotifyOfPropertyChange(() => this.AudioEncoders);
            this.NotifyOfPropertyChange(() => this.OutputFormat);
            this.CorrectAudioEncoders(this.OutputFormat);
        }

        public void LaunchHelp()
        {
            Process.Start("explorer.exe", "https://handbrake.fr/docs/en/latest/advanced/audio-subtitle-defaults.html");
        }

        #endregion

        public bool ShowWindow()
        {
            this.IsApplied = false;
            this.windowManager.ShowDialog<AudioDefaultsView>(this);

            return this.IsApplied;
        }

        public void Cancel()
        {
            this.IsApplied = false;
            this.TryClose();
        }

        public void Save()
        {
            this.audioBehaviours.AllowedPassthruOptions =
                this.PassthruEncoders.Where(s => s.IsEnabled).Select(s => s.Encoder).ToList();
            this.IsApplied = true;
            this.TryClose(true);
        }

        private void CorrectAudioEncoders(OutputFormat outputFormat)
        {
            if (outputFormat == OutputFormat.Mp4 && this.AudioEncoderFallback != null && !this.AudioEncoderFallback.SupportsMP4)
            {
                this.AudioEncoderFallback = HandBrakeEncoderHelpers.GetAudioEncoder(HBAudioEncoder.AvAac);
            }

            if (outputFormat == OutputFormat.WebM && this.AudioEncoderFallback != null && !this.AudioEncoderFallback.SupportsWebM)
            {
                this.AudioEncoderFallback = HandBrakeEncoderHelpers.GetAudioEncoder(HBAudioEncoder.Vorbis);
            }

            if (outputFormat == OutputFormat.Mp4)
            {
                foreach (AudioBehaviourTrack track in this.BehaviourTracks.Where(track => !track.Encoder.SupportsMP4))
                {
                    track.Encoder = HandBrakeEncoderHelpers.GetAudioEncoder(HBAudioEncoder.AvAac);
                }
            }

            if (outputFormat == OutputFormat.WebM)
            {
                foreach (AudioBehaviourTrack track in this.BehaviourTracks.Where(track => !track.Encoder.SupportsWebM))
                {
                    track.Encoder = HandBrakeEncoderHelpers.GetAudioEncoder(HBAudioEncoder.Vorbis);
                }
            }
        }
    }
}