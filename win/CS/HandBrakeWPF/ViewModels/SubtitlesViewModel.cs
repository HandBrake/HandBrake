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
    using System.Windows;

    using HandBrake.Interop.Interop;
    using HandBrake.Interop.Interop.Interfaces.Model;

    using HandBrake.Interop.Utilities;

    using HandBrakeWPF.Commands;
    using HandBrakeWPF.EventArgs;
    using HandBrakeWPF.Model.Subtitles;
    using HandBrakeWPF.Properties;
    using HandBrakeWPF.Services.Interfaces;
    using HandBrakeWPF.Services.Presets.Model;
    using HandBrakeWPF.Services.Scan.Model;
    using HandBrakeWPF.Utilities.FileDialogs;
    using HandBrakeWPF.ViewModels.Interfaces;

    using EncodeTask = Services.Encode.Model.EncodeTask;
    using OutputFormat = Services.Encode.Model.Models.OutputFormat;
    using SubtitleTrack = Services.Encode.Model.Models.SubtitleTrack;
    using SubtitleType = Services.Encode.Model.Models.SubtitleType;

    /// <summary>
    /// The Subtitles View Model
    /// </summary>
    public class SubtitlesViewModel : ViewModelBase, ISubtitlesViewModel
    {
        private readonly IErrorService errorService;

        private readonly ISubtitleRuleProcessor subtitleRuleProcessor;
        private readonly ISubtitleFileHandler subtitleFileHandler;

        private readonly Subtitle foreignAudioSearchTrack;
        private IList<Subtitle> sourceTracks;
        
        /// <summary>
        /// Initializes a new instance of the <see cref="HandBrakeWPF.ViewModels.SubtitlesViewModel"/> class.
        /// </summary>
        /// <param name="errorService">
        /// The Error Service
        /// </param>
        /// <param name="windowManager">
        /// The window Manager.
        /// </param>
        public SubtitlesViewModel(IErrorService errorService, IWindowManager windowManager, ISubtitleRuleProcessor subtitleRuleProcessor, ISubtitleFileHandler subtitleFileHandler)
        {
            this.errorService = errorService;
            this.subtitleRuleProcessor = subtitleRuleProcessor;
            this.subtitleFileHandler = subtitleFileHandler;
            this.SubtitleBehaviours = new SubtitleBehaviourRule();
            this.SubtitleDefaultsViewModel = new SubtitlesDefaultsViewModel(windowManager);
            this.Task = new EncodeTask();

            this.Languages = HandBrakeLanguagesHelper.AllLanguagesWithAny;
            this.CharacterCodes = CharCodesUtilities.GetCharacterCodes();

            this.foreignAudioSearchTrack = new Subtitle { IsFakeForeignAudioScanTrack = true, Language = Resources.SubtitleViewModel_ForeignAudioSearch };
            this.SourceTracks = new List<Subtitle> { this.foreignAudioSearchTrack };
            
            this.SetBurnedToFalseForAllExceptCommand = new SimpleRelayCommand<SubtitleTrack>(this.SetBurnedToFalseForAllExcept);
            this.SelectDefaultTrackCommand = new SimpleRelayCommand<SubtitleTrack>(this.SelectDefaultTrack);
            this.RemoveTrackCommand = new SimpleRelayCommand<SubtitleTrack>(this.Remove);
        }

        public SimpleRelayCommand<SubtitleTrack> SetBurnedToFalseForAllExceptCommand { get; }
        public SimpleRelayCommand<SubtitleTrack> SelectDefaultTrackCommand { get; }
        public SimpleRelayCommand<SubtitleTrack> RemoveTrackCommand { get; }

        public event EventHandler<TabStatusEventArgs> TabStatusChanged;

        /// <summary>
        /// Gets or sets the audio defaults view model.
        /// </summary>
        public ISubtitlesDefaultsViewModel SubtitleDefaultsViewModel { get; set; }

        public ListboxDeleteCommand DeleteCommand => new ListboxDeleteCommand();

        /// <summary>
        /// Gets or sets CharacterCodes.
        /// </summary>
        public IEnumerable<string> CharacterCodes { get; set; }

        /// <summary>
        /// Gets or sets Languages.
        /// </summary>
        public IEnumerable<Language> Languages { get; set; }

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
        /// Gets the default audio behaviours. 
        /// </summary>
        public SubtitleBehaviourRule SubtitleBehaviours { get; private set; }
        
        public bool IsBurnableOnly => this.Task.OutputFormat == OutputFormat.WebM;

        public void AddUser()
        {
            int count = this.Task.SubtitleTracks.Count;
            this.Add();
            this.CheckAddState(count);
        }

        public void AddAllRemainingUser()
        {
            int count = this.Task.SubtitleTracks.Count;
            this.AddAllRemaining();
            this.CheckAddState(count);
        }

        public void AddAllClosedCaptionsUser()
        {
            int count = this.Task.SubtitleTracks.Count;
            this.AddAllClosedCaptions();
            this.CheckAddState(count);
        }

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
            foreach (Subtitle subtitle in this.SourceTitlesSubset(null).Where(s => s.SubtitleType == SubtitleType.CC608 || s.SubtitleType == SubtitleType.CC708))
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
                if (subtitle.IsFakeForeignAudioScanTrack)
                {
                    continue;
                }

                this.Add(subtitle);
            }
        }

        /// <summary>
        /// Import an SRT File.
        /// </summary>
        public void ImportSubtitle()
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

            if (dialog.FileNames != null)
            {
                foreach (SubtitleTrack track in this.subtitleFileHandler.GetInputSubtitles(dialog.FileNames, true))
                {
                    this.Task.SubtitleTracks.Add(track);
                }
            }
        }

        public void Import(string[] subtitleFiles)
        {
            if (subtitleFiles != null && subtitleFiles.Any())
            {
                foreach (SubtitleTrack track in this.subtitleFileHandler.GetInputSubtitles(subtitleFiles, true))
                {
                    this.Task.SubtitleTracks.Add(track);
                }
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

            List<SubtitleTrack> newTracks = this.subtitleRuleProcessor.GenerateTrackList(this.SubtitleBehaviours, this.SourceTracks.ToList(), this.Task.OutputFormat, this.Task.Source);
            foreach (SubtitleTrack track in newTracks)
            {
                this.Task.SubtitleTracks.Add(track);
            }
        }

        /// <summary>
        /// The show audio defaults.
        /// </summary>
        public void ShowSubtitleDefaultsPanel()
        {
            this.SubtitleDefaultsViewModel.SetupPreset(this.SubtitleBehaviours);
            if (this.SubtitleDefaultsViewModel.ShowWindow())
            {
                this.SubtitleBehaviours = new SubtitleBehaviourRule(this.SubtitleDefaultsViewModel.SubtitleBehaviourRules);
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

            this.SubtitleBehaviours = preset.SubtitleTrackBehaviours != null ? new SubtitleBehaviourRule(preset.SubtitleTrackBehaviours) : new SubtitleBehaviourRule();
            
            this.SubtitleDefaultsViewModel.SetupPreset(preset);
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
            if (preset.SubtitleTrackBehaviours != null && this.SubtitleBehaviours != null)
            {
                if (this.SubtitleBehaviours.Equals(preset.SubtitleTrackBehaviours))
                {
                    return true;
                }
            }
            else
            {
                return true; // Presets probably havn't been reset so we'll treat this functionality as unavailable. 
            }

            return false;
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

        protected virtual void OnTabStatusChanged(TabStatusEventArgs e)
        {
            this.TabStatusChanged?.Invoke(this, e);
        }

        /// <summary>
        /// Add a subtitle track.
        /// The Source track is set based on the following order. If null, it will skip to the next option.
        ///   1. Passed in Subtitle param
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
                                             s => !s.IsFakeForeignAudioScanTrack))
                                      : null);

            if (source == null)
            {
                source = foreignAudioSearchTrack;

                if (this.Task.SubtitleTracks.Any(s => s.SourceTrack.Equals(this.foreignAudioSearchTrack)))
                {
                    return; // Don't add more than one Foreign Audio Scan
                }
            }


            SubtitleTrack track = new SubtitleTrack
                                      {
                                          SubtitleType = source.SubtitleType,
                                          SourceTrack = source,
                                      };

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
                   // this.SetBurnedToFalseForAllExcept(track);
                }
            }

            var encodeTask = this.Task;
            if (encodeTask != null)
            {
                encodeTask.SubtitleTracks.Add(track);
            }
        }

        /// <summary>
        /// The get preferred subtitle track, or the first if none available.
        /// </summary>
        /// <returns>
        /// The users preferred language, or the first if none available.
        /// </returns>
        private string GetPreferredSubtitleTrackLanguage()
        {
            List<Language> languages = subtitleRuleProcessor.GetRequestedHumanLanguages(this.SubtitleBehaviours);

            Language langName = languages.FirstOrDefault(w => w != HandBrakeLanguagesHelper.AnyLanguage);
            return langName?.Code;
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

        private void CheckAddState(int before)
        {
            if (this.Task.SubtitleTracks.Count == before)
            {
                this.errorService.ShowMessageBox(
                    Resources.SubtitleView_NoSubtitlesAdded,
                    Resources.Info,
                    MessageBoxButton.OK,
                    MessageBoxImage.Information);
            }
        }
    }
}
