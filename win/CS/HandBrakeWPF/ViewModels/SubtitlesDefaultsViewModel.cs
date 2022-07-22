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

    using HandBrake.App.Core.Utilities;
    using HandBrake.Interop.Utilities;

    using HandBrakeWPF.Model.Subtitles;
    using HandBrakeWPF.Properties;
    using HandBrakeWPF.Services.Interfaces;
    using HandBrakeWPF.Services.Presets.Model;
    using HandBrakeWPF.ViewModels.Interfaces;
    using HandBrakeWPF.Views;

    /// <summary>
    /// The Subtitles View Model
    /// </summary>
    public class SubtitlesDefaultsViewModel : ViewModelBase, ISubtitlesDefaultsViewModel
    {
        private readonly IWindowManager windowManager;

        private SubtitleBehaviours subtitleBehaviours;
        private BindingList<string> availableLanguages;

        /// <summary>
        /// Initializes a new instance of the <see cref="SubtitlesDefaultsViewModel"/> class. 
        /// </summary>
        public SubtitlesDefaultsViewModel(IWindowManager windowManager)
        {
            this.windowManager = windowManager;
            this.Languages = LanguageUtilities.MapLanguages().Keys;
            this.CharacterCodes = CharCodesUtilities.GetCharacterCodes();
            this.SubtitleBehaviours = new SubtitleBehaviours();
            this.SelectedAvailableToMove = new BindingList<string>();
            this.SelectedLanguagesToMove = new BindingList<string>();
            this.availableLanguages = new BindingList<string>();
            this.SetupPreset((Preset)null);

            this.Title = Resources.SubtitlesViewModel_SubDefaults;
        }

        public bool IsApplied { get; set; }

        /// <summary>
        /// Gets CharacterCodes.
        /// </summary>
        public IEnumerable<string> CharacterCodes { get; private set; }

        /// <summary>
        /// Gets Languages.
        /// </summary>
        public IEnumerable<string> Languages { get; private set; }

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
        /// Gets the subtitle behaviour modes.
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
                this.NotifyOfPropertyChange(() => this.AvailableLanguages);
            }
        }

        /// <summary>
        /// Gets SelectedLanguages.
        /// </summary>
        public BindingList<string> SelectedAvailableToMove { get; private set; }

        /// <summary>
        /// Gets SelectedLanguages.
        /// </summary>
        public BindingList<string> SelectedLanguagesToMove { get; private set; }
        
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
                    this.SubtitleBehaviours.SelectedLanguages.Add(item);
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
                List<string> copiedList = SelectedLanguagesToMove.ToList();
                foreach (string item in copiedList)
                {
                    this.SubtitleBehaviours.SelectedLanguages.Remove(item);
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
            foreach (string item in this.SubtitleBehaviours.SelectedLanguages)
            {
                this.AvailableLanguages.Add(item);
            }
            this.AvailableLanguages = new BindingList<string>(this.AvailableLanguages.OrderBy(o => o).ToList());

            this.SubtitleBehaviours.SelectedLanguages.Clear();
        }

        public void LaunchHelp()
        {
            Process.Start("explorer.exe", "https://handbrake.fr/docs/en/latest/advanced/audio-subtitle-defaults.html");
        }

        /// <summary>
        /// The setup languages.
        /// </summary>
        /// <param name="preset">
        /// The preset.
        /// </param>
        public void SetupPreset(Preset preset)
        {
            if (preset != null)
            {
                this.SetupPreset(preset.SubtitleTrackBehaviours);
            }
        }

        /// <summary>
        /// The setup languages.
        /// </summary>
        /// <param name="behaviours">
        /// The behaviours.
        /// </param>
        public void SetupPreset(SubtitleBehaviours behaviours)
        {
            // Reset
            this.IsApplied = false;

            // Step 1, Set the behaviour mode
            this.SubtitleBehaviours.SelectedBehaviour = SubtitleBehaviourModes.None;
            this.SubtitleBehaviours.SelectedBurnInBehaviour = SubtitleBurnInBehaviourModes.None;
            this.SubtitleBehaviours.AddClosedCaptions = false;
            this.SubtitleBehaviours.AddForeignAudioScanTrack = false;
            this.SubtitleBehaviours.SelectedLanguages.Clear();

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

                foreach (string selectedItem in behaviours.SelectedLanguages)
                {
                    this.AvailableLanguages.Remove(selectedItem);
                    this.SubtitleBehaviours.SelectedLanguages.Add(selectedItem);
                }
            }
        }

        public bool ShowWindow()
        {
            this.IsApplied = false;
            this.windowManager.ShowDialog<SubtitlesDefaultsView>(this);

            return this.IsApplied;
        }

        public void Cancel()
        {
            this.IsApplied = false;
            this.TryClose();
        }

        public void Save()
        {
            this.IsApplied = true;
            this.TryClose();
        }
    }
}