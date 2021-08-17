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
    using System.Windows.Navigation;

    using Caliburn.Micro;

    using HandBrake.Interop.Interop;
    using HandBrake.Interop.Utilities;

    using HandBrakeWPF.Model.Audio;
    using HandBrakeWPF.Properties;
    using HandBrakeWPF.Services.Encode.Model;
    using HandBrakeWPF.Services.Encode.Model.Models;
    using HandBrakeWPF.Services.Presets.Model;
    using HandBrakeWPF.Utilities;
    using HandBrakeWPF.ViewModels.Interfaces;

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
            this.AudioEncoders = EnumHelper<AudioEncoder>.GetEnumList();

            this.SampleRates = new ObservableCollection<string> { "Auto" };
            foreach (var item in HandBrakeEncoderHelpers.AudioSampleRates)
            {
                this.SampleRates.Add(item.Name);
            }

            this.Title = Resources.AudioViewModel_AudioDefaults;
        }

        #region Properties

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

        public bool AudioAllowMP2Pass
        {
            get
            {
                return this.audioBehaviours.AllowedPassthruOptions.AudioAllowMP2Pass;
            }

            set
            {
                this.audioBehaviours.AllowedPassthruOptions.AudioAllowMP2Pass = value;
                this.NotifyOfPropertyChange(() => this.AudioAllowMP2Pass);
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether audio allow m p 3 pass.
        /// </summary>
        public bool AudioAllowMP3Pass
        {
            get
            {
                return this.audioBehaviours.AllowedPassthruOptions.AudioAllowMP3Pass;
            }

            set
            {
                this.audioBehaviours.AllowedPassthruOptions.AudioAllowMP3Pass = value;
                this.NotifyOfPropertyChange(() => this.AudioAllowMP3Pass);
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether audio allow aac pass.
        /// </summary>
        public bool AudioAllowAACPass
        {
            get
            {
                return this.audioBehaviours.AllowedPassthruOptions.AudioAllowAACPass;
            }

            set
            {
                this.audioBehaviours.AllowedPassthruOptions.AudioAllowAACPass = value;
                this.NotifyOfPropertyChange(() => this.AudioAllowAACPass);
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether audio allow ac3 pass.
        /// </summary>
        public bool AudioAllowAC3Pass
        {
            get
            {
                return this.audioBehaviours.AllowedPassthruOptions.AudioAllowAC3Pass;
            }

            set
            {
                this.audioBehaviours.AllowedPassthruOptions.AudioAllowAC3Pass = value;
                this.NotifyOfPropertyChange(() => this.AudioAllowAC3Pass);
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether audio allow eac3 pass.
        /// </summary>
        public bool AudioAllowEAC3Pass
        {
            get
            {
                return this.audioBehaviours.AllowedPassthruOptions.AudioAllowEAC3Pass;
            }

            set
            {
                this.audioBehaviours.AllowedPassthruOptions.AudioAllowEAC3Pass = value;
                this.NotifyOfPropertyChange(() => this.AudioAllowEAC3Pass);
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether audio allow dts pass.
        /// </summary>
        public bool AudioAllowDTSPass
        {
            get
            {
                return this.audioBehaviours.AllowedPassthruOptions.AudioAllowDTSPass;
            }

            set
            {
                this.audioBehaviours.AllowedPassthruOptions.AudioAllowDTSPass = value;
                this.NotifyOfPropertyChange(() => this.AudioAllowDTSPass);
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether audio allow dtshd pass.
        /// </summary>
        public bool AudioAllowDTSHDPass
        {
            get
            {
                return this.audioBehaviours.AllowedPassthruOptions.AudioAllowDTSHDPass;
            }

            set
            {
                this.audioBehaviours.AllowedPassthruOptions.AudioAllowDTSHDPass = value;
                this.NotifyOfPropertyChange(() => this.AudioAllowDTSHDPass);
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether audio allow true hd pass.
        /// </summary>
        public bool AudioAllowTrueHDPass
        {
            get
            {
                return this.audioBehaviours.AllowedPassthruOptions.AudioAllowTrueHDPass;
            }

            set
            {
                this.audioBehaviours.AllowedPassthruOptions.AudioAllowTrueHDPass = value;
                this.NotifyOfPropertyChange(() => this.AudioAllowTrueHDPass);
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether audio allow flac pass.
        /// </summary>
        public bool AudioAllowFlacPass
        {
            get
            {
                return this.audioBehaviours.AllowedPassthruOptions.AudioAllowFlacPass;
            }

            set
            {
                this.audioBehaviours.AllowedPassthruOptions.AudioAllowFlacPass = value;
                this.NotifyOfPropertyChange(() => this.AudioAllowFlacPass);
            }
        }

        /// <summary>
        /// Gets or sets the audio encoder fallback.
        /// </summary>
        public AudioEncoder AudioEncoderFallback
        {
            get
            {
                return this.audioBehaviours.AllowedPassthruOptions.AudioEncoderFallback;
            }

            set
            {
                if (value == this.audioBehaviours.AllowedPassthruOptions.AudioEncoderFallback)
                {
                    return;
                }

                foreach (var item in this.BehaviourTracks)
                {
                    item.SetFallbackEncoder(value);
                }

                this.audioBehaviours.AllowedPassthruOptions.AudioEncoderFallback = value;
                this.NotifyOfPropertyChange(() => this.AudioEncoderFallback);
            }
        }

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
        public IEnumerable<AudioEncoder> AudioEncoders { get; set; }

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
                this.audioBehaviours.AllowedPassthruOptions = behaviours.AllowedPassthruOptions;
             
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

            this.NotifyOfPropertyChange(() => this.AudioAllowMP2Pass);
            this.NotifyOfPropertyChange(() => this.AudioAllowMP3Pass);
            this.NotifyOfPropertyChange(() => this.AudioAllowAACPass);
            this.NotifyOfPropertyChange(() => this.AudioAllowAC3Pass);
            this.NotifyOfPropertyChange(() => this.AudioAllowEAC3Pass);
            this.NotifyOfPropertyChange(() => this.AudioAllowDTSPass);
            this.NotifyOfPropertyChange(() => this.AudioAllowDTSHDPass);
            this.NotifyOfPropertyChange(() => this.AudioAllowTrueHDPass);
            this.NotifyOfPropertyChange(() => this.AudioAllowFlacPass);
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
            this.windowManager.ShowDialogAsync(this);

            return this.IsApplied;
        }

        public void Cancel()
        {
            this.IsApplied = false;
        }

        public void Save()
        {
            this.IsApplied = true;
            this.TryCloseAsync(false);
        }


        private void CorrectAudioEncoders(OutputFormat outputFormat)
        {
            if (outputFormat == OutputFormat.Mp4 &&
                (this.AudioEncoderFallback == AudioEncoder.ffflac || this.AudioEncoderFallback == AudioEncoder.ffflac24 || this.AudioEncoderFallback == AudioEncoder.Vorbis || this.AudioEncoderFallback == AudioEncoder.Opus))
            {
                this.AudioEncoderFallback = AudioEncoder.ffaac;
            }

            if (outputFormat == OutputFormat.WebM &&
                (this.AudioEncoderFallback != AudioEncoder.Opus && this.AudioEncoderFallback != AudioEncoder.Vorbis))
            {
                this.AudioEncoderFallback = AudioEncoder.Vorbis;
            }

            if (outputFormat == OutputFormat.Mp4)
            {
                foreach (AudioBehaviourTrack track in this.BehaviourTracks.Where(track => track.Encoder == AudioEncoder.ffflac || track.Encoder == AudioEncoder.ffflac24 || track.Encoder == AudioEncoder.Opus || track.Encoder == AudioEncoder.Vorbis))
                {
                    track.Encoder = AudioEncoder.ffaac;
                }
            }

            if (outputFormat == OutputFormat.WebM)
            {
                foreach (AudioBehaviourTrack track in this.BehaviourTracks.Where(track => track.Encoder != AudioEncoder.Vorbis && track.Encoder != AudioEncoder.Opus))
                {
                    track.Encoder = AudioEncoder.Vorbis;
                }
            }
        }
    }
}