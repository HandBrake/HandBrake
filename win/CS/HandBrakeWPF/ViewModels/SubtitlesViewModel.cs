// --------------------------------------------------------------------------------------------------------------------
// <copyright file="SubtitlesViewModel.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The Subtitles View Model
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.ViewModels
{
    using System.Collections.Generic;
    using System.ComponentModel;
    using System.IO;
    using System.Linq;

    using HandBrake.ApplicationServices.Services.Encode.Model;
    using HandBrake.ApplicationServices.Services.Encode.Model.Models;
    using HandBrake.ApplicationServices.Services.Scan.Model;
    using HandBrake.ApplicationServices.Utilities;

    using HandBrakeWPF.Model.Subtitles;
    using HandBrakeWPF.Properties;
    using HandBrakeWPF.Services.Presets.Model;
    using HandBrakeWPF.ViewModels.Interfaces;

    using Microsoft.Win32;

    /// <summary>
    /// The Subtitles View Model
    /// </summary>
    public class SubtitlesViewModel : ViewModelBase, ISubtitlesViewModel
    {
        #region Constants and Fields

        /// <summary>
        /// The Foreign Audio Search Track
        /// </summary>
        private readonly Subtitle ForeignAudioSearchTrack;

        /// <summary>
        /// Backing field for the source subtitle tracks.
        /// </summary>
        private IList<Subtitle> sourceTracks;

        /// <summary>
        /// The show defaults panel.
        /// </summary>
        private bool showDefaultsPanel;

        /// <summary>
        /// The audio behaviours.
        /// </summary>
        private SubtitleBehaviours subtitleBehaviours;

        /// <summary>
        /// The available languages.
        /// </summary>
        private BindingList<string> availableLanguages;

        #endregion

        #region Constructors and Destructors

        /// <summary>
        /// Initializes a new instance of the <see cref="HandBrakeWPF.ViewModels.SubtitlesViewModel"/> class.
        /// </summary>
        public SubtitlesViewModel()
        {
            this.Task = new EncodeTask();

            this.Langauges = LanguageUtilities.MapLanguages().Keys;
            this.CharacterCodes = CharCodesUtilities.GetCharacterCodes();

            this.ForeignAudioSearchTrack = new Subtitle { SubtitleType = SubtitleType.ForeignAudioSearch, Language = "Foreign Audio Search (Bitmap)" };
            this.SourceTracks = new List<Subtitle> { this.ForeignAudioSearchTrack };

            this.SubtitleBehaviours = new SubtitleBehaviours();
            this.SelectedAvailableToMove = new BindingList<string>();
            this.SelectedLangaugesToMove = new BindingList<string>();
            this.availableLanguages = new BindingList<string>();
            this.SetupLanguages(null);
        }

        #endregion

        #region Properties

        /// <summary>
        /// Gets or sets CharacterCodes.
        /// </summary>
        public IEnumerable<string> CharacterCodes { get; set; }

        /// <summary>
        /// Gets or sets Langauges.
        /// </summary>
        public IEnumerable<string> Langauges { get; set; }

        /// <summary>
        /// Gets or sets SourceTracks.
        /// </summary>
        public IList<Subtitle> SourceTracks
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
        /// Gets or sets Task.
        /// </summary>
        public EncodeTask Task { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether show defaults panel.
        /// </summary>
        public bool ShowDefaultsPanel
        {
            get
            {
                return this.showDefaultsPanel;
            }
            set
            {
                if (value.Equals(this.showDefaultsPanel))
                {
                    return;
                }
                this.showDefaultsPanel = value;
                this.NotifyOfPropertyChange(() => this.ShowDefaultsPanel);
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
                return this.ShowDefaultsPanel ? Resources.SubtitlesViewModel_SubDefaults : Resources.SubtitlesViewModel_SubTracks;
            }
        }

        /// <summary>
        /// Gets the switch display title.
        /// </summary>
        public string SwitchDisplayTitle
        {
            get
            {
                return this.ShowDefaultsPanel ? Resources.SubtitlesViewModel_SwitchToTracks : Resources.SubtitlesViewModel_ConfigureDefaults;
            }
        }

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
        /// Add a new Track
        /// </summary>
        public void Add()
        {
            this.Add(null);
        }

        /// <summary>
        /// Add all closed captions not already on the list.
        /// </summary>
        public void AddAllClosedCaptions()
        {
            foreach (Subtitle subtitle in this.SourceTitlesSubset(null).Where(s => s.SubtitleType == SubtitleType.CC))
            {
                this.Add(subtitle);
            }
        }

        /// <summary>
        /// Add all the remaining subtitle tracks.
        /// </summary>
        public void AddAllRemaining()
        {
            foreach (Subtitle subtitle in this.SourceTitlesSubset(null))
            {
                this.Add(subtitle);
            }
        }

        /// <summary>
        /// Add all remaining tracks for the users preferred and selected languages
        /// </summary>
        public void AddAllRemainingForSelectedLanguages()
        {
            // Translate to Iso Codes
            List<string> iso6392Codes = this.SubtitleBehaviours.SelectedLangauges.Contains(Constants.Any)
                                            ? LanguageUtilities.GetIsoCodes()
                                            : LanguageUtilities.GetLanguageCodes(
                                                this.SubtitleBehaviours.SelectedLangauges.ToArray());
                                 
            List<Subtitle> availableTracks =
                this.SourceTracks.Where(subtitle => iso6392Codes.Contains(subtitle.LanguageCodeClean)).ToList();

            foreach (Subtitle subtitle in this.SourceTitlesSubset(availableTracks))
            {
                this.Add(subtitle);
            }
        }

        /// <summary>
        /// The add first for selected languages.
        /// </summary>
        private void AddFirstForSelectedLanguages()
        {
            foreach (Subtitle sourceTrack in this.GetSelectedLanguagesTracks())
            {
                // Step 2: Check if the track list already contrains this track
                bool found = this.Task.SubtitleTracks.Any(track => Equals(track.SourceTrack, sourceTrack));
                if (!found)
                {
                    // Check if we are already using this language
                    bool foundLanguage = false;
                    Subtitle track = sourceTrack;

                    foreach (var item in this.Task.SubtitleTracks)
                    {
                        if (item.SourceTrack != null && item.SourceTrack.LanguageCode != null && track.LanguageCode.Contains(item.SourceTrack.LanguageCode))
                        {
                            foundLanguage = true;
                        }     
                    }

                    if (foundLanguage)
                    {
                        continue;
                    }

                    // If it doesn't, add it.
                    this.Add(sourceTrack);
                }
            }
        }

        /// <summary>
        /// Import an SRT File.
        /// </summary>
        public void Import()
        {
            OpenFileDialog dialog = new OpenFileDialog
                {
                    Filter = "SRT files (*.srt)|*.srt",
                    CheckFileExists = true,
                    Multiselect = true
                };

            dialog.ShowDialog();

            foreach (var srtFile in dialog.FileNames)
            {
                SubtitleTrack track = new SubtitleTrack
                    {
                        SrtFileName = Path.GetFileNameWithoutExtension(srtFile),
                        SrtOffset = 0,
                        SrtCharCode = "UTF-8",
                        SrtLang = "English",
                        SubtitleType = SubtitleType.SRT,
                        SrtPath = srtFile
                    };
                this.Task.SubtitleTracks.Add(track);
            }
        }

        /// <summary>
        /// Remove a Track
        /// </summary>
        /// <param name="track">
        /// The track.
        /// </param>
        public void Remove(SubtitleTrack track)
        {
            this.Task.SubtitleTracks.Remove(track);
        }

        /// <summary>
        /// Clear all Tracks
        /// </summary>
        public void Clear()
        {
            this.Task.SubtitleTracks.Clear();
        }

        /// <summary>
        /// Select the default subtitle track.
        /// </summary>
        /// <param name="subtitle">
        /// The subtitle.
        /// </param>
        public void SelectDefaultTrack(SubtitleTrack subtitle)
        {
            foreach (SubtitleTrack track in this.Task.SubtitleTracks)
            {
                if (track == subtitle)
                {
                    continue; // Skip the track the user selected.
                }
                track.Default = false;
            }

            this.NotifyOfPropertyChange(() => this.Task);
        }

        /// <summary>
        /// Select the burned in track.
        /// </summary>
        /// <param name="subtitle">
        /// The subtitle.
        /// </param>
        public void SetBurnedToFalseForAllExcept(SubtitleTrack subtitle)
        {
            foreach (SubtitleTrack track in this.Task.SubtitleTracks)
            {
                if (track == subtitle)
                {
                    continue; // Skip the track the user selected.
                }
                track.Burned = false;
            }
            this.NotifyOfPropertyChange(() => this.Task);
        }

        /// <summary>
        /// Automatic Subtitle Selection based on user preferences.
        /// </summary>
        public void AutomaticSubtitleSelection()
        {
            this.Task.SubtitleTracks.Clear();

            // Add Foreign Audio Scan
            if (this.SubtitleBehaviours.AddForeignAudioScanTrack)
            {
                this.Add(ForeignAudioSearchTrack);
            }

            // Add Track Behaviours
            switch (this.SubtitleBehaviours.SelectedBehaviour)
            {
                case SubtitleBehaviourModes.FirstMatch: // Adding all remaining tracks
                    this.AddFirstForSelectedLanguages();
                    break;
                case SubtitleBehaviourModes.AllMatching: // Add Langauges tracks for the additional languages selected, in-order.
                    this.AddAllRemainingForSelectedLanguages();
                    break;
            }

            // Burn In Behaviour
            if (this.Task.SubtitleTracks.Count >= 1)
            {
                bool burnInSet = false;
                switch (this.SubtitleBehaviours.SelectedBurnInBehaviour)
                {
                    case SubtitleBurnInBehaviourModes.None:
                        // Do Nothing. Only tracks where the container requires it will be burned in.
                        break;
                    case SubtitleBurnInBehaviourModes.ForeignAudio:
                        foreach (var track in this.Task.SubtitleTracks)
                        {
                            // Set the Foreign Audio Track to burned-in
                            if (track.SourceTrack.SubtitleType == SubtitleType.ForeignAudioSearch)
                            {
                                track.Burned = true;
                                this.SetBurnedToFalseForAllExcept(track);
                                break;
                            }
                        }
                        break;
                    case SubtitleBurnInBehaviourModes.FirstTrack:                    
                        foreach (var track in this.Task.SubtitleTracks)
                        {
                            // Set the first track.
                            if (!burnInSet && track.SourceTrack.SubtitleType != SubtitleType.ForeignAudioSearch)
                            {
                                burnInSet = true;
                                track.Burned = true;
                                this.SetBurnedToFalseForAllExcept(track);
                            }
                        }                  
                        break;
                    case SubtitleBurnInBehaviourModes.ForeignAudioPreferred:
                        foreach (var track in this.Task.SubtitleTracks)
                        {
                            // Set the first track.
                            if (!burnInSet)
                            {
                                burnInSet = true;
                                track.Burned = true;
                                this.SetBurnedToFalseForAllExcept(track);
                            }

                            // But if there is a foreign audio track, prefer this to the first.
                            if (track.SourceTrack.SubtitleType == SubtitleType.ForeignAudioSearch)
                            {
                                track.Burned = true;
                                this.SetBurnedToFalseForAllExcept(track);
                                break;
                            }
                        }            
                        break;
                }
            }

            // Add all closed captions if enabled.
            if (this.SubtitleBehaviours.AddClosedCaptions)
            {
                this.AddAllClosedCaptions();
            }
        }

        /// <summary>
        /// Open the options screen to the Audio and Subtitles tab.
        /// </summary>
        public void SetDefaultBehaviour()
        {
            this.ShowDefaultsPanel = true;
        }

        /// <summary>
        /// The show audio defaults.
        /// </summary>
        public void ShowSubtitleDefaultsPanel()
        {
            this.ShowDefaultsPanel = !this.ShowDefaultsPanel;
        }

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

        /// <summary>
        /// Reload the audio tracks based on the defaults.
        /// </summary>
        public void ReloadDefaults()
        {
            this.AutomaticSubtitleSelection();
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
            // Note, We don't support Subtitles in presets yet.
            this.Task = task;
            this.NotifyOfPropertyChange(() => this.Task);

            this.SetupLanguages(preset);
            this.AutomaticSubtitleSelection();
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
            this.NotifyOfPropertyChange(() => this.Task.SubtitleTracks);
            this.NotifyOfPropertyChange(() => this.Task);
        }

        /// <summary>
        /// Setup this window for a new source
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
            this.SourceTracks.Clear();
            this.SourceTracks.Add(ForeignAudioSearchTrack);
            foreach (Subtitle subtitle in title.Subtitles)
            {
                this.SourceTracks.Add(subtitle);
            }

            this.Task = task;
            this.NotifyOfPropertyChange(() => this.Task);

            this.AutomaticSubtitleSelection();
        }

        #endregion

        #region Methods

        /// <summary>
        /// Add a subtitle track.
        /// The Source track is set based on the following order. If null, it will skip to the next option.
        ///   1. Passed in Subitle param
        ///   2. First preferred Subtitle from source
        ///   3. First subtitle from source.
        /// Will not add a subtitle if the source has none.
        /// </summary>
        /// <param name="subtitle">
        /// The subtitle. Use null to add preferred, or first from source (based on user preference)
        /// </param>
        private void Add(Subtitle subtitle)
        {
            Subtitle source = subtitle
                              ?? ((this.SourceTracks != null)
                                      ? (this.SourceTracks.FirstOrDefault(l => l.Language == this.GetPreferredSubtitleTrackLanguage())
                                         ?? this.SourceTracks.FirstOrDefault(
                                             s => s.SubtitleType != SubtitleType.ForeignAudioSearch))
                                      : null);

            if (source == null)
            {
                source = ForeignAudioSearchTrack;
            }

            SubtitleTrack track = new SubtitleTrack
                                      {
                                          SubtitleType = SubtitleType.VobSub,
                                          SourceTrack = source,
                                      };

            // Burn-in Behaviours
            if (this.SubtitleBehaviours.SelectedBurnInBehaviour == SubtitleBurnInBehaviourModes.ForeignAudio
                  || this.SubtitleBehaviours.SelectedBurnInBehaviour == SubtitleBurnInBehaviourModes.ForeignAudioPreferred)
            {
                track.Burned = true;
                this.SetBurnedToFalseForAllExcept(track);
            }

            // For MP4, PGS Subtitles must be burned in.
            if (!track.Burned && (source.SubtitleType == SubtitleType.PGS) && this.Task != null && this.Task.OutputFormat == OutputFormat.Mp4)
            {
                if (track.CanBeBurned)
                {
                    track.Burned = true;
                    this.SetBurnedToFalseForAllExcept(track);
                }
            }

            var encodeTask = this.Task;
            if (encodeTask != null)
            {
                encodeTask.SubtitleTracks.Add(track);
            }
        }

        /// <summary>
        /// Gets a list of source tracks for the users selected languages.
        /// </summary>
        /// <returns>
        /// A list of source subtitle tracks.
        /// </returns>
        private IEnumerable<Subtitle> GetSelectedLanguagesTracks()
        {
            List<Subtitle> trackList = new List<Subtitle>();

            List<string> isoCodes = this.SubtitleBehaviours.SelectedLangauges.Contains(Constants.Any)
                                            ? LanguageUtilities.GetIsoCodes()
                                            : LanguageUtilities.GetLanguageCodes(
                                                this.SubtitleBehaviours.SelectedLangauges.ToArray());

            foreach (string code in isoCodes)
            {
                trackList.AddRange(this.SourceTracks.Where(source => source.LanguageCode == code));
            }

            return trackList;
        }

        /// <summary>
        /// The get preferred subtitle track, or the first if none available.
        /// </summary>
        /// <returns>
        /// The users preferred language, or the first if none available.
        /// </returns>
        private string GetPreferredSubtitleTrackLanguage()
        {
            return this.SubtitleBehaviours.SelectedLangauges.FirstOrDefault(w => w != Constants.Any);
        }

        /// <summary>
        /// Gets a list of Source subtitle tracks that are not currently used.
        /// </summary>
        /// <param name="subtitles">
        /// The subtitles. (Optional).  If null, works on the full source subtitle collection
        /// </param>
        /// <returns>
        /// An IEnumerable collection of subtitles
        /// </returns>
        private IEnumerable<Subtitle> SourceTitlesSubset(IEnumerable<Subtitle> subtitles)
        {
            return subtitles != null
                       ? subtitles.Where(subtitle => !this.Task.SubtitleTracks.Any(track => Equals(track.SourceTrack, subtitle))).ToList()
                       : this.SourceTracks.Where(subtitle => !this.Task.SubtitleTracks.Any(track => Equals(track.SourceTrack, subtitle))).ToList();
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
            if (preset != null && preset.SubtitleTrackBehaviours != null)
            {
                this.SubtitleBehaviours.SelectedBehaviour = preset.SubtitleTrackBehaviours.SelectedBehaviour;
                this.SubtitleBehaviours.SelectedBurnInBehaviour = preset.SubtitleTrackBehaviours.SelectedBurnInBehaviour;
                this.SubtitleBehaviours.AddClosedCaptions = preset.SubtitleTrackBehaviours.AddClosedCaptions;
                this.SubtitleBehaviours.AddForeignAudioScanTrack = preset.SubtitleTrackBehaviours.AddForeignAudioScanTrack;

                foreach (string selectedItem in preset.SubtitleTrackBehaviours.SelectedLangauges)
                {
                    this.AvailableLanguages.Remove(selectedItem);
                    this.SubtitleBehaviours.SelectedLangauges.Add(selectedItem);
                }
            }
        }
        #endregion
    }
}