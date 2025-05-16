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
    using System;
    using System.Collections.Generic;
    using System.ComponentModel;
    using System.Diagnostics;
    using System.Linq;

    using HandBrake.App.Core.Utilities;
    using HandBrake.Interop.Interop;
    using HandBrake.Interop.Interop.Interfaces.Model;
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
        private BindingList<Language> availableLanguages;

        /// <summary>
        /// Initializes a new instance of the <see cref="SubtitlesDefaultsViewModel"/> class. 
        /// </summary>
        public SubtitlesDefaultsViewModel(IWindowManager windowManager)
        {
            this.windowManager = windowManager;
            this.Languages = HandBrakeLanguagesHelper.AllLanguagesWithAny;
            this.CharacterCodes = CharCodesUtilities.GetCharacterCodes();
            this.SubtitleBehaviours = new SubtitleBehaviours();
            this.SelectedAvailableToMove = new BindingList<Language>();
            this.SelectedLanguagesToMove = new BindingList<Language>();
            this.availableLanguages = new BindingList<Language>();
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
        public IEnumerable<Language> Languages { get; private set; }

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
        public BindingList<Language> AvailableLanguages
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
        public BindingList<Language> SelectedAvailableToMove { get; private set; }

        /// <summary>
        /// Gets SelectedLanguages.
        /// </summary>
        public BindingList<Language> SelectedLanguagesToMove { get; private set; }
        
        /// <summary>
        /// Audio List Move Left
        /// </summary>
        public void LanguageMoveRight()
        {
            if (this.SelectedAvailableToMove.Count > 0)
            {
                List<Language> copiedList = SelectedAvailableToMove.ToList();
                foreach (Language item in copiedList)
                {
                    this.SubtitleBehaviours.SelectedLanguages.Add(item);
                }

                this.UpdateAvailableLanguages();
            }
        }

        /// <summary>
        /// Audio List Move Right
        /// </summary>
        public void LanguageMoveLeft()
        {
            if (this.SelectedLanguagesToMove.Count > 0)
            {
                List<Language> copiedList = SelectedLanguagesToMove.ToList();
                foreach (Language item in copiedList)
                {
                    this.SubtitleBehaviours.SelectedLanguages.Remove(item);
                }
            }

            this.UpdateAvailableLanguages();
        }

        /// <summary>
        /// Language List Clear all selected languages
        /// </summary>
        public void LanguageClearAll()
        {
            this.SubtitleBehaviours.SelectedLanguages.Clear();
            this.UpdateAvailableLanguages();
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

            // Step 2, Setup Available Languages
            this.AvailableLanguages.Clear();
            foreach (Language item in HandBrakeLanguagesHelper.AllLanguagesWithAny)
            {
                this.AvailableLanguages.Add(item);
            }

            // Step 3, Set the Selected Languages        
            if (behaviours != null)
            {
                this.SubtitleBehaviours.SelectedBehaviour = behaviours.SelectedBehaviour;
                this.SubtitleBehaviours.SelectedBurnInBehaviour = behaviours.SelectedBurnInBehaviour;
                this.SubtitleBehaviours.AddClosedCaptions = behaviours.AddClosedCaptions;
                this.SubtitleBehaviours.AddForeignAudioScanTrack = behaviours.AddForeignAudioScanTrack;
                this.SubtitleBehaviours.SubtitleTrackNamePassthru = behaviours.SubtitleTrackNamePassthru;

                foreach (Language selectedItem in behaviours.SelectedLanguages)
                {
                    this.SubtitleBehaviours.SelectedLanguages.Add(selectedItem);
                }

                this.UpdateAvailableLanguages();
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

        private void UpdateAvailableLanguages()
        {
            List<Language> copiedList = this.SubtitleBehaviours.SelectedLanguages.ToList();

            BindingList<Language> newAvailable = new BindingList<Language>();

            foreach (Language lang in HandBrakeLanguagesHelper.AllLanguagesWithAny)
            {
                if (!copiedList.Contains(lang))
                {
                    newAvailable.Add(lang);
                }
            }

            this.AvailableLanguages = newAvailable;
        }
    }
}