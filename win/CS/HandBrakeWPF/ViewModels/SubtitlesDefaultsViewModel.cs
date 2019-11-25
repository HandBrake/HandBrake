// --------------------------------------------------------------------------------------------------------------------
// <copyright file="SubtitlesDefaultsViewModel.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The Subtitles Defaults View Model
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.ViewModels
{
    using System.Collections.Generic;
    using System.ComponentModel;
    using System.Diagnostics;
    using System.Linq;

    using HandBrake.Interop.Utilities;

    using HandBrakeWPF.Model.Subtitles;
    using HandBrakeWPF.Properties;
    using HandBrakeWPF.Services.Presets.Model;
    using HandBrakeWPF.Utilities;
    using HandBrakeWPF.ViewModels.Interfaces;

    /// <summary>
    /// The Subtitles View Model
    /// </summary>
    public class SubtitlesDefaultsViewModel : ViewModelBase, ISubtitlesDefaultsViewModel
    {
        #region Constants and Fields

        private SubtitleBehaviours subtitleBehaviours;
        private BindingList<string> availableLanguages;

        #endregion

        #region Constructors and Destructors

        /// <summary>
        /// Initializes a new instance of the <see cref="SubtitlesDefaultsViewModel"/> class. 
        /// </summary>
        public SubtitlesDefaultsViewModel()
        {
            this.Langauges = LanguageUtilities.MapLanguages().Keys;
            this.CharacterCodes = CharCodesUtilities.GetCharacterCodes();
            this.SubtitleBehaviours = new SubtitleBehaviours();
            this.SelectedAvailableToMove = new BindingList<string>();
            this.SelectedLangaugesToMove = new BindingList<string>();
            this.availableLanguages = new BindingList<string>();
            this.SetupLanguages((Preset)null);

            this.Title = Resources.SubtitlesViewModel_SubDefaults;
        }

        #endregion

        #region Properties

        public bool IsApplied { get; set; }

        /// <summary>
        /// Gets CharacterCodes.
        /// </summary>
        public IEnumerable<string> CharacterCodes { get; private set; }

        /// <summary>
        /// Gets Langauges.
        /// </summary>
        public IEnumerable<string> Langauges { get; private set; }

        /// <summary>
        /// Gets or sets the subtitle behaviours.
        /// </summary>
        public SubtitleBehaviours SubtitleBehaviours
        {
            get
            {
                return this.subtitleBehaviours;
            }
            set
            {
                if (Equals(value, this.subtitleBehaviours))
                {
                    return;
                }

                this.subtitleBehaviours = value;
                this.NotifyOfPropertyChange(() => this.SubtitleBehaviours);
            }
        }

        /// <summary>
        /// Gets the sbutitle behaviour modes.
        /// </summary>
        public BindingList<SubtitleBehaviourModes> SubtitleBehaviourModeList
        {
            get
            {
                return new BindingList<SubtitleBehaviourModes>(EnumHelper<SubtitleBehaviourModes>.GetEnumList().ToList());
            }
        }

        /// <summary>
        /// Gets the subtitle burn in behaviour mode list.
        /// </summary>
        public BindingList<SubtitleBurnInBehaviourModes> SubtitleBurnInBehaviourModeList
        {
            get
            {
                return new BindingList<SubtitleBurnInBehaviourModes>(EnumHelper<SubtitleBurnInBehaviourModes>.GetEnumList().ToList());
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
                List<string> copiedList = SelectedAvailableToMove.ToList();
                foreach (string item in copiedList)
                {
                    this.AvailableLanguages.Remove(item);
                    this.SubtitleBehaviours.SelectedLangauges.Add(item);
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
                List<string> copiedList = SelectedLangaugesToMove.ToList();
                foreach (string item in copiedList)
                {
                    this.SubtitleBehaviours.SelectedLangauges.Remove(item);
                    this.AvailableLanguages.Add(item);
                }
            }

            this.AvailableLanguages = new BindingList<string>(this.AvailableLanguages.OrderBy(o => o).ToList());
        }

        /// <summary>
        /// Language List Clear all selected languages
        /// </summary>
        public void LanguageClearAll()
        {
            foreach (string item in this.SubtitleBehaviours.SelectedLangauges)
            {
                this.AvailableLanguages.Add(item);
            }
            this.AvailableLanguages = new BindingList<string>(this.AvailableLanguages.OrderBy(o => o).ToList());

            this.SubtitleBehaviours.SelectedLangauges.Clear();
        }

        public void LaunchHelp()
        {
            Process.Start("https://handbrake.fr/docs/en/1.2.0/advanced/audio-subtitle-defaults.html");
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
                this.SetupLanguages(preset.SubtitleTrackBehaviours);
            }
        }

        public void ResetApplied()
        {
            this.IsApplied = false;
        }

        /// <summary>
        /// The setup languages.
        /// </summary>
        /// <param name="behaviours">
        /// The behaviours.
        /// </param>
        public void SetupLanguages(SubtitleBehaviours behaviours)
        {
            // Reset
            this.IsApplied = false;

            // Step 1, Set the behaviour mode
            this.SubtitleBehaviours.SelectedBehaviour = SubtitleBehaviourModes.None;
            this.SubtitleBehaviours.SelectedBurnInBehaviour = SubtitleBurnInBehaviourModes.None;
            this.SubtitleBehaviours.AddClosedCaptions = false;
            this.SubtitleBehaviours.AddForeignAudioScanTrack = false;
            this.SubtitleBehaviours.SelectedLangauges.Clear();

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
                this.SubtitleBehaviours.SelectedBehaviour = behaviours.SelectedBehaviour;
                this.SubtitleBehaviours.SelectedBurnInBehaviour = behaviours.SelectedBurnInBehaviour;
                this.SubtitleBehaviours.AddClosedCaptions = behaviours.AddClosedCaptions;
                this.SubtitleBehaviours.AddForeignAudioScanTrack = behaviours.AddForeignAudioScanTrack;

                foreach (string selectedItem in behaviours.SelectedLangauges)
                {
                    this.AvailableLanguages.Remove(selectedItem);
                    this.SubtitleBehaviours.SelectedLangauges.Add(selectedItem);
                }
            }
        }
        #endregion
    }
}