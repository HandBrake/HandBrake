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

    using HandBrake.Interop.Interop;
    using HandBrake.Interop.Interop.Model.Encoding;
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
        private BindingList<string> availableLanguages;
        private AudioBehaviours audioBehaviours;
        private EncodeTask task;
        #region Constructors and Destructors

        /// <summary>
        /// Initializes a new instance of the <see cref="AudioDefaultsViewModel"/> class.
        /// </summary>
        /// <param name="task">
        /// The task.
        /// </param>
        public AudioDefaultsViewModel(EncodeTask task)
        {
            this.Task = task;
            this.AudioBehaviours = new AudioBehaviours();
            this.SelectedAvailableToMove = new BindingList<string>();
            this.SelectedLangaugesToMove = new BindingList<string>();
            this.AvailableLanguages = new BindingList<string>();
            this.AudioEncoders = EnumHelper<AudioEncoder>.GetEnumList();

            this.SampleRates = new ObservableCollection<string> { "Auto" };
            foreach (var item in HandBrakeEncoderHelpers.AudioSampleRates)
            {
                this.SampleRates.Add(item.Name);
            }

            this.Setup((Preset)null, task);

            this.Title = Resources.AudioViewModel_AudioDefaults;
        }

        #endregion

        #region Properties

        /// <summary>
        /// Gets or sets the task.
        /// </summary>
        public EncodeTask Task
        {
            get
            {
                return this.task;
            }
            set
            {
                if (Equals(value, this.task))
                {
                    return;
                }
                this.task = value;
                this.NotifyOfPropertyChange(() => this.Task);
            }
        }

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
        /// Gets SelectedLangauges.
        /// </summary>
        public BindingList<string> SelectedAvailableToMove { get; private set; }

        /// <summary>
        /// Gets SelectedLangauges.
        /// </summary>
        public BindingList<string> SelectedLangaugesToMove { get; private set; }

        /// <summary>
        /// Gets or sets a value indicating whether audio allow m p 3 pass.
        /// </summary>
        public bool AudioAllowMP3Pass
        {
            get
            {
                return this.Task.AllowedPassthruOptions.AudioAllowMP3Pass;
            }

            set
            {
                this.task.AllowedPassthruOptions.AudioAllowMP3Pass = value;
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
                return this.Task.AllowedPassthruOptions.AudioAllowAACPass;
            }

            set
            {
                this.task.AllowedPassthruOptions.AudioAllowAACPass = value;
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
                return this.Task.AllowedPassthruOptions.AudioAllowAC3Pass;
            }

            set
            {
                this.task.AllowedPassthruOptions.AudioAllowAC3Pass = value;
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
                return this.Task.AllowedPassthruOptions.AudioAllowEAC3Pass;
            }

            set
            {
                this.task.AllowedPassthruOptions.AudioAllowEAC3Pass = value;
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
                return this.Task.AllowedPassthruOptions.AudioAllowDTSPass;
            }

            set
            {
                this.task.AllowedPassthruOptions.AudioAllowDTSPass = value;
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
                return this.Task.AllowedPassthruOptions.AudioAllowDTSHDPass;
            }

            set
            {
                this.task.AllowedPassthruOptions.AudioAllowDTSHDPass = value;
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
                return this.Task.AllowedPassthruOptions.AudioAllowTrueHDPass;
            }

            set
            {
                this.task.AllowedPassthruOptions.AudioAllowTrueHDPass = value;
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
                return this.Task.AllowedPassthruOptions.AudioAllowFlacPass;
            }

            set
            {
                this.task.AllowedPassthruOptions.AudioAllowFlacPass = value;
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
                return this.Task.AllowedPassthruOptions.AudioEncoderFallback;
            }
            set
            {
                if (value == this.Task.AllowedPassthruOptions.AudioEncoderFallback)
                {
                    return;
                }
                this.Task.AllowedPassthruOptions.AudioEncoderFallback = value;
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
                this.NotifyOfPropertyChange("AvailableLanguages");
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
            this.BehaviourTracks.AddNew();
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

        public void ResetApplied()
        {
            this.IsApplied = false;
        }

        #endregion

        #region Methods

        /// <summary>
        /// The setup languages.
        /// </summary>
        /// <param name="preset">
        /// The preset.
        /// </param>
        /// <param name="task">
        /// The task.
        /// </param>
        public void Setup(Preset preset, EncodeTask task)
        {
            // Reset
            this.IsApplied = false;
            this.AudioBehaviours = new AudioBehaviours();

            // Setup for this Encode Task.
            this.Task = task;

            IDictionary<string, string> langList = LanguageUtilities.MapLanguages();
            langList = (from entry in langList orderby entry.Key ascending select entry).ToDictionary(pair => pair.Key, pair => pair.Value);

            this.AvailableLanguages.Clear();
            foreach (string item in langList.Keys)
            {
                this.AvailableLanguages.Add(item);
            }

            // Handle the Preset, if it's not null.
            if (preset == null)
            {
                return;
            }
           
            AudioBehaviours behaviours = preset.AudioTrackBehaviours.Clone();
            if (behaviours != null)
            {
                this.AudioBehaviours.SelectedBehaviour = behaviours.SelectedBehaviour;
                this.AudioBehaviours.SelectedTrackDefaultBehaviour = behaviours.SelectedTrackDefaultBehaviour;

                foreach (AudioBehaviourTrack item in preset.AudioTrackBehaviours.BehaviourTracks)
                {
                    this.BehaviourTracks.Add(new AudioBehaviourTrack(item));
                }

                this.NotifyOfPropertyChange(() => this.BehaviourTracks);
                
                foreach (string selectedItem in behaviours.SelectedLangauges)
                {
                    this.AvailableLanguages.Remove(selectedItem);
                    this.AudioBehaviours.SelectedLangauges.Add(selectedItem);
                }
            }

            this.task.AllowedPassthruOptions = new AllowedPassthru(preset.Task.AllowedPassthruOptions);

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
        public void RefreshTask()
        {
            this.NotifyOfPropertyChange(() => this.Task);

            if (this.Task.OutputFormat == OutputFormat.Mp4 && 
                (this.AudioEncoderFallback == AudioEncoder.ffflac || this.AudioEncoderFallback == AudioEncoder.ffflac24 || this.AudioEncoderFallback == AudioEncoder.Vorbis))
            {
                    this.AudioEncoderFallback = AudioEncoder.ffaac;
            }
        }

        public void LaunchHelp()
        {
            Process.Start("https://handbrake.fr/docs/en/1.2.0/advanced/audio-subtitle-defaults.html");
        }
        
        #endregion
    }
}