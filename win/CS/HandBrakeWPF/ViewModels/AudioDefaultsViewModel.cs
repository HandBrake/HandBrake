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
    using System.ComponentModel;
    using System.Linq;

    using HandBrake.ApplicationServices.Utilities;

    using HandBrakeWPF.Model.Audio;
    using HandBrakeWPF.Services.Presets.Model;
    using HandBrakeWPF.Utilities;
    using HandBrakeWPF.ViewModels.Interfaces;

    /// <summary>
    /// The Audio View Model
    /// </summary>
    public class AudioDefaultsViewModel : ViewModelBase, IAudioDefaultsViewModel
    {
        private BindingList<string> availableLanguages;
        private AudioBehaviours audioBehaviours;

        #region Constructors and Destructors

        /// <summary>
        /// Initializes a new instance of the <see cref="AudioDefaultsViewModel"/> class.
        /// </summary>
        public AudioDefaultsViewModel()
        {
            this.AudioBehaviours = new AudioBehaviours();
            this.SelectedAvailableToMove = new BindingList<string>();
            this.SelectedLangaugesToMove = new BindingList<string>();
            this.AvailableLanguages = new BindingList<string>();
            this.SetupLanguages((Preset)null);
        }

        #endregion

        #region Properties

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
        /// Gets SelectedLangauges.
        /// </summary>
        public BindingList<string> SelectedAvailableToMove { get; private set; }

        /// <summary>
        /// Gets SelectedLangauges.
        /// </summary>
        public BindingList<string> SelectedLangaugesToMove { get; private set; }

        #endregion

        #region Public Methods

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

        #region Methods

        /// <summary>
        /// The setup languages.
        /// </summary>
        /// <param name="preset">
        /// The preset.
        /// </param>
        public void SetupLanguages(Preset preset)
        {
            if (preset != null)
            {
                this.SetupLanguages(preset.AudioTrackBehaviours.Clone());
            }
        }

        /// <summary>
        /// The setup languages.
        /// </summary>
        /// <param name="behaviours">
        /// The behaviours.
        /// </param>
        public void SetupLanguages(AudioBehaviours behaviours)
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
            if (behaviours != null)
            {
                this.AudioBehaviours.SelectedBehaviour = behaviours.SelectedBehaviour;
                this.AudioBehaviours.SelectedTrackDefaultBehaviour = behaviours.SelectedTrackDefaultBehaviour;

                foreach (string selectedItem in behaviours.SelectedLangauges)
                {
                    this.AvailableLanguages.Remove(selectedItem);
                    this.AudioBehaviours.SelectedLangauges.Add(selectedItem);
                }
            }
        }

        #endregion
    }
}