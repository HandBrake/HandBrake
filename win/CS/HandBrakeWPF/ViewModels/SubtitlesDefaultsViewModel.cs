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
    using HandBrake.Interop.Interop;
    using HandBrake.Interop.Interop.Interfaces.Model;
    using HandBrake.Interop.Utilities;

    using HandBrakeWPF.Commands;
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

        private BindingList<Language> availableLanguages;

        private SubtitleBehaviourRule subtitleBehaviourRules;
        
        /// <summary>
        /// Initializes a new instance of the <see cref="SubtitlesDefaultsViewModel"/> class. 
        /// </summary>
        public SubtitlesDefaultsViewModel(IWindowManager windowManager)
        {
            this.windowManager = windowManager;
            this.Languages = HandBrakeLanguagesHelper.AllLanguagesWithAny;
            this.CharacterCodes = CharCodesUtilities.GetCharacterCodes();
            this.SelectedAvailableToMove = new BindingList<Language>();
            this.SelectedLanguagesToMove = new BindingList<Language>();
            this.availableLanguages = new BindingList<Language>();
            this.SubtitleBehaviourRules = new SubtitleBehaviourRule();
            
            this.SetupPreset((Preset)null);

            this.Title = Resources.SubtitlesViewModel_SubDefaults;

            this.RemoveTrackCommand = new SimpleRelayCommand<SubtitleBehaviourTrack>(this.RemoveTrack, null);
        }

        public SubtitleBehaviourRule SubtitleBehaviourRules
        {
            get => this.subtitleBehaviourRules;
            private set
            {
                if (Equals(value, this.subtitleBehaviourRules))
                {
                    return;
                }

                this.subtitleBehaviourRules = value;
                this.NotifyOfPropertyChange(() => this.SubtitleBehaviourRules);
            }
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

        public BindingList<SubtitleBehaviourModes> TrackSelectionModes => new BindingList<SubtitleBehaviourModes>(EnumHelper<SubtitleBehaviourModes>.GetEnumList().ToList());

        public BindingList<IsDefaultModes> IsDefaultModes => new BindingList<IsDefaultModes>(EnumHelper<IsDefaultModes>.GetEnumList().ToList());

        public BindingList<SubtitleBurnInBehaviourModes> BurnPassthruModes => new BindingList<SubtitleBurnInBehaviourModes>(EnumHelper<SubtitleBurnInBehaviourModes>.GetEnumList().ToList());

        public BindingList<ForcedModes> ForcedModes => new BindingList<ForcedModes>(EnumHelper<ForcedModes>.GetEnumList().ToList());

        public SimpleRelayCommand<SubtitleBehaviourTrack> RemoveTrackCommand { get; }

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

        public void AddRule()
        {
            this.SubtitleBehaviourRules.Tracks.Add(new SubtitleBehaviourTrack());
        }

        public void AddForeignScanTrack()
        {
            this.SubtitleBehaviourRules.Tracks.Add(new SubtitleBehaviourTrack() { IsForeignAudioScanRule = true});
        }

        /// <summary>
        /// Remove the Selected Track
        /// </summary>
        /// <param name="track">
        /// The track.
        /// </param>
        public void RemoveTrack(SubtitleBehaviourTrack track)
        {
            this.SubtitleBehaviourRules.Tracks.Remove(track);
        }

        public void Clear()
        {
            this.SubtitleBehaviourRules.Tracks.Clear();
        }

        /// <summary>
        /// The setup languages.
        /// </summary>
        /// <param name="behaviours">
        /// The behaviours.
        /// </param>
        public void SetupPreset(SubtitleBehaviourRule behaviours)
        {
            // Reset
            this.IsApplied = false;

            // Step 1, Set the behaviour mode
            this.SubtitleBehaviourRules = new SubtitleBehaviourRule(behaviours);
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