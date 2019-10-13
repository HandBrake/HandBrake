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
    using System;
    using System.Collections.Generic;
    using System.IO;
    using System.Linq;
    using System.Runtime.CompilerServices;
    using System.Windows;

    using Caliburn.Micro;

    using HandBrake.Interop.Utilities;

    using HandBrakeWPF.EventArgs;
    using HandBrakeWPF.Model.Subtitles;
    using HandBrakeWPF.Properties;
    using HandBrakeWPF.Services.Interfaces;
    using HandBrakeWPF.Services.Presets.Model;
    using HandBrakeWPF.Services.Scan.Model;
    using HandBrakeWPF.ViewModels.Interfaces;
    using HandBrakeWPF.Views;

    using Microsoft.Win32;

    using EncodeTask = HandBrakeWPF.Services.Encode.Model.EncodeTask;
    using OutputFormat = HandBrakeWPF.Services.Encode.Model.Models.OutputFormat;
    using SubtitleTrack = HandBrakeWPF.Services.Encode.Model.Models.SubtitleTrack;
    using SubtitleType = HandBrakeWPF.Services.Encode.Model.Models.SubtitleType;

    /// <summary>
    /// The Subtitles View Model
    /// </summary>
    public class SubtitlesViewModel : ViewModelBase, ISubtitlesViewModel
    {
        private readonly IErrorService errorService;
        private readonly IWindowManager windowManager;

        #region Constants and Fields

        private readonly Subtitle foreignAudioSearchTrack;
        private IList<Subtitle> sourceTracks;

        #endregion

        #region Constructors and Destructors

        /// <summary>
        /// Initializes a new instance of the <see cref="HandBrakeWPF.ViewModels.SubtitlesViewModel"/> class.
        /// </summary>
        /// <param name="errorService">
        /// The Error Service
        /// </param>
        /// <param name="windowManager">
        /// The window Manager.
        /// </param>
        public SubtitlesViewModel(IErrorService errorService, IWindowManager windowManager)
        {
            this.errorService = errorService;
            this.windowManager = windowManager;
            this.SubtitleDefaultsViewModel = new SubtitlesDefaultsViewModel();
            this.Task = new EncodeTask();

            this.Langauges = LanguageUtilities.MapLanguages().Keys;
            this.CharacterCodes = CharCodesUtilities.GetCharacterCodes();

            this.foreignAudioSearchTrack = new Subtitle { SubtitleType = SubtitleType.ForeignAudioSearch, Language = Resources.SubtitleViewModel_ForeignAudioSearch };
            this.SourceTracks = new List<Subtitle> { this.foreignAudioSearchTrack };
        }

        #endregion

        public event EventHandler<TabStatusEventArgs> TabStatusChanged;

        #region Properties

        /// <summary>
        /// Gets or sets the audio defaults view model.
        /// </summary>
        public ISubtitlesDefaultsViewModel SubtitleDefaultsViewModel { get; set; }

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
        /// Gets the panel title.
        /// </summary>
        public string PanelTitle
        {
            get
            {
                return Resources.SubtitlesViewModel_SubTracks;
            }
        }

        /// <summary>
        /// Gets the switch display title.
        /// </summary>
        public string SwitchDisplayTitle
        {
            get
            {
                return Resources.SubtitlesViewModel_ConfigureDefaults;
            }
        }

        /// <summary>
        /// Gets the default audio behaviours. 
        /// </summary>
        public SubtitleBehaviours SubtitleBehaviours
        {
            get
            {
                return this.SubtitleDefaultsViewModel.SubtitleBehaviours;
            }
        }

        public bool IsBurnableOnly
        {
            get
            {
                return this.Task.OutputFormat == OutputFormat.WebM;
            }
        }

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
            List<Subtitle> availableTracks = this.GetSelectedLanguagesTracks();

            foreach (Subtitle subtitle in this.SourceTitlesSubset(availableTracks))
            {
                this.Add(subtitle);
            }
        }

        /// <summary>
        /// The add first for selected languages.
        /// </summary>
        public void AddFirstForSelectedLanguages()
        {
            bool anyLanguageSelected = this.SubtitleBehaviours.SelectedLangauges.Contains(Constants.Any);
            foreach (Subtitle sourceTrack in this.GetSelectedLanguagesTracks())
            {
                // Step 2: Check if the track list already contains this track
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

                    // If we are using "(Any)" then break here. We only add the first track in this instance.
                    if (anyLanguageSelected)
                    {
                        break;
                    }
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
                Filter = "Subtitle files (*.srt, *.ssa, *.ass)|*.srt;*.ssa;*.ass",
                CheckFileExists = true,
                Multiselect = true
            };

            if (this.Task != null && this.Task.Source != null)
            {
                string path = Path.GetDirectoryName(this.Task.Source);
                if (Directory.Exists(path))
                {
                    dialog.InitialDirectory = path;
                }
            }

            dialog.ShowDialog();

            this.AddInputSubtitles(dialog.FileNames);
        }

        public void Import(string[] subtitleFiles)
        {
            if (subtitleFiles != null && subtitleFiles.Any())
            {
                this.AddInputSubtitles(subtitleFiles);
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
                this.Add(foreignAudioSearchTrack);
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
                        foreach (var track in this.Task.SubtitleTracks)
                        {
                            if (track.SourceTrack.SubtitleType == SubtitleType.ForeignAudioSearch)
                            {
                                track.Forced = true;
                                break;
                            }
                        }

                        break;
                    case SubtitleBurnInBehaviourModes.ForeignAudio:
                        foreach (var track in this.Task.SubtitleTracks)
                        {
                            // Set the Foreign Audio Track to burned-in
                            if (track.SourceTrack.SubtitleType == SubtitleType.ForeignAudioSearch)
                            {
                                track.Burned = true;
                                track.Forced = true;
                                this.SetBurnedToFalseForAllExcept(track);
                                break;
                            }
                        }

                        break;
                    case SubtitleBurnInBehaviourModes.FirstTrack:                    
                        foreach (var track in this.Task.SubtitleTracks)
                        {
                            // Foreign Audio Search is always first in the list.
                            if (track.SourceTrack.SubtitleType == SubtitleType.ForeignAudioSearch) 
                            {
                                track.Forced = true;
                                continue;
                            }

                            if (!burnInSet)
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
                                track.Forced = true;
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
        /// The show audio defaults.
        /// </summary>
        public void ShowSubtitleDefaultsPanel()
        {
            SubtitlesDefaultsView view = new SubtitlesDefaultsView();
            view.DataContext = this.SubtitleDefaultsViewModel;

            if (view.ShowDialog() == true)
            {
                this.OnTabStatusChanged(null);
            }
        }

        /// <summary>
        /// Reload the audio tracks based on the defaults.
        /// </summary>
        public void ReloadDefaults()
        {
            this.AutomaticSubtitleSelection();
        }

        /// <summary>
        /// Trigger a Notify Property Changed on the Task to force various UI elements to update.
        /// </summary>
        public void RefreshTask()
        {
            this.NotifyOfPropertyChange(() => this.Task);
            this.NotifyOfPropertyChange(() => this.IsBurnableOnly);

            if (this.IsBurnableOnly)
            {
                foreach (var subtitleTrack in this.Task.SubtitleTracks)
                {
                    if (subtitleTrack.Default)
                    {
                        subtitleTrack.Default = false;
                    }
                }
            }
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

            this.SubtitleDefaultsViewModel.SetupLanguages(preset);
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

        public bool MatchesPreset(Preset preset)
        {
            // Check the default behaviours.
            if (preset.SubtitleTrackBehaviours.AddClosedCaptions != this.SubtitleBehaviours.AddClosedCaptions)
            {
                return false;
            }

            if (preset.SubtitleTrackBehaviours.AddForeignAudioScanTrack != this.SubtitleBehaviours.AddForeignAudioScanTrack)
            {
                return false;
            }

            if (preset.SubtitleTrackBehaviours.SelectedBehaviour != this.SubtitleBehaviours.SelectedBehaviour)
            {
                return false;
            }

            if (preset.SubtitleTrackBehaviours.SelectedBurnInBehaviour != this.SubtitleBehaviours.SelectedBurnInBehaviour)
            {
                return false;
            }

            foreach (var item in this.SubtitleBehaviours.SelectedLangauges)
            {
                if (!preset.SubtitleTrackBehaviours.SelectedLangauges.Contains(item))
                {
                    return false;
                }
            }

            return true;
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
            this.SourceTracks.Add(foreignAudioSearchTrack);
            foreach (Subtitle subtitle in title.Subtitles)
            {
                this.SourceTracks.Add(subtitle);
            }

            this.Task = task;
            this.NotifyOfPropertyChange(() => this.Task);

            this.AutomaticSubtitleSelection();
        }

        /// <summary>
        /// Checks the configuration of the subtitles and warns the user about any potential issues.
        /// </summary>
        public bool ValidateSubtitles()
        {
            var nonBurnedSubtitles = this.Task.SubtitleTracks.Where(subtitleTrack => !subtitleTrack.Burned).ToList();

            if (nonBurnedSubtitles.Count > 0 && this.IsBurnableOnly)
            {
                MessageBoxResult result = this.errorService.ShowMessageBox(
                    Resources.Subtitles_WebmSubtitleIncompatibilityError,
                    Resources.Subtitles_WebmSubtitleIncompatibilityHeader,
                    MessageBoxButton.OKCancel,
                    MessageBoxImage.Warning);
                if (result == MessageBoxResult.OK)
                {
                    foreach (var subtitleTrack in nonBurnedSubtitles)
                    {
                        if (!subtitleTrack.Burned)
                        {
                            this.Remove(subtitleTrack);
                        }
                    }
                }
                else if (result == MessageBoxResult.Cancel)
                {
                    return false;
                }
                else
                {
                    return false;
                }
            }

            return true;
        }

        #endregion

        #region Methods

        protected virtual void OnTabStatusChanged(TabStatusEventArgs e)
        {
            this.TabStatusChanged?.Invoke(this, e);
        }

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
                source = foreignAudioSearchTrack;
            }

            SubtitleTrack track = new SubtitleTrack
                                      {
                                          SubtitleType = source.SubtitleType,
                                          SourceTrack = source,
                                      };

            // Burn-in Behaviours
            if (this.SubtitleBehaviours.SelectedBurnInBehaviour == SubtitleBurnInBehaviourModes.ForeignAudio
                  || this.SubtitleBehaviours.SelectedBurnInBehaviour == SubtitleBurnInBehaviourModes.ForeignAudioPreferred)
            {
                if (subtitle != null && subtitle.SubtitleType == SubtitleType.ForeignAudioSearch)
                {
                    track.Burned = true;
                    this.SetBurnedToFalseForAllExcept(track);
                }
            }

            // For MP4, PGS Subtitles must be burned in.
            if (!track.Burned && (source.SubtitleType == SubtitleType.PGS) && this.Task != null && this.Task.OutputFormat == OutputFormat.Mp4)
            {
                if (this.Task.SubtitleTracks.Any(a => a.Burned))
                {
                    return; // We can't add any more burned in tracks.
                }

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
        private List<Subtitle> GetSelectedLanguagesTracks()
        {
            // Translate to Iso Codes
            List<string> iso6392Codes = this.SubtitleBehaviours.SelectedLangauges.Contains(Constants.Any)
                ? LanguageUtilities.GetIsoCodes()
                : LanguageUtilities.GetLanguageCodes(
                    this.SubtitleBehaviours.SelectedLangauges.ToArray());


            List<Subtitle> orderedSubtitles = new List<Subtitle>();
            foreach (string code in iso6392Codes)
            {
                orderedSubtitles.AddRange(this.SourceTracks.Where(subtitle => subtitle.LanguageCodeClean == code ));
            }

            return orderedSubtitles;
        }

        /// <summary>
        /// The get preferred subtitle track, or the first if none available.
        /// </summary>
        /// <returns>
        /// The users preferred language, or the first if none available.
        /// </returns>
        private string GetPreferredSubtitleTrackLanguage()
        {
            string langName = this.SubtitleBehaviours.SelectedLangauges.FirstOrDefault(w => w != Constants.Any);
            string langCode = LanguageUtilities.GetLanguageCode(langName);
            return langCode;
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

        private void AddInputSubtitles(string[] filenames)
        {
            foreach (var srtFile in filenames)
            {
                if (!File.Exists(srtFile))
                {
                    continue;
                }

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

        #endregion
    }
}
