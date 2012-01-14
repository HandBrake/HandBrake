// --------------------------------------------------------------------------------------------------------------------
// <copyright file="OptionsViewModel.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The Options View Model
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.ViewModels
{
    using System;
    using System.Collections.Generic;
    using System.Collections.Specialized;
    using System.ComponentModel;
    using System.ComponentModel.Composition;
    using System.Diagnostics;
    using System.Globalization;
    using System.IO;
    using System.Linq;
    using System.Windows;

    using Caliburn.Micro;

    using HandBrake.ApplicationServices;
    using HandBrake.ApplicationServices.Services.Interfaces;
    using HandBrake.ApplicationServices.Utilities;

    using HandBrakeWPF.ViewModels.Interfaces;

    using Ookii.Dialogs.Wpf;

    /// <summary>
    /// The Options View Model
    /// </summary>
    [Export(typeof(IOptionsViewModel))]
    public class OptionsViewModel : ViewModelBase, IOptionsViewModel
    {
        #region Constants and Fields

        /// <summary>
        /// Backing field for the user setting service.
        /// </summary>
        private readonly IUserSettingService userSettingService;

        /// <summary>
        /// The add audio mode options.
        /// </summary>
        private BindingList<string> addAudioModeOptions = new BindingList<string>();

        /// <summary>
        /// The add closed captions.
        /// </summary>
        private bool addClosedCaptions;

        /// <summary>
        /// The add only one audio track per language.
        /// </summary>
        private bool addOnlyOneAudioTrackPerLanguage;

        /// <summary>
        /// The add subtitle mode options.
        /// </summary>
        private BindingList<string> addSubtitleModeOptions = new BindingList<string>();

        /// <summary>
        /// The arguments.
        /// </summary>
        private string arguments;

        /// <summary>
        /// The auto name default path.
        /// </summary>
        private string autoNameDefaultPath;

        /// <summary>
        /// The automatically name files.
        /// </summary>
        private bool automaticallyNameFiles;

        /// <summary>
        /// The autoname format.
        /// </summary>
        private string autonameFormat;

        /// <summary>
        /// The available languages.
        /// </summary>
        private BindingList<string> availableLanguages = new BindingList<string>();

        /// <summary>
        /// The change to title case.
        /// </summary>
        private bool changeToTitleCase;

        /// <summary>
        /// The check for updates.
        /// </summary>
        private bool checkForUpdates;

        /// <summary>
        /// The check for updates frequencies.
        /// </summary>
        private BindingList<string> checkForUpdatesFrequencies = new BindingList<string>();

        /// <summary>
        /// The check for updates frequency.
        /// </summary>
        private int checkForUpdatesFrequency;

        /// <summary>
        /// The clear old olgs.
        /// </summary>
        private bool clearOldOlgs;

        /// <summary>
        /// The constant quality granularity.
        /// </summary>
        private BindingList<string> constantQualityGranularity = new BindingList<string>();

        /// <summary>
        /// The copy log to encode directory.
        /// </summary>
        private bool copyLogToEncodeDirectory;

        /// <summary>
        /// The copy log to sepcficed location.
        /// </summary>
        private bool copyLogToSepcficedLocation;

        /// <summary>
        /// The disable libdvd nav.
        /// </summary>
        private bool disableLibdvdNav;

        /// <summary>
        /// The disable p reset update check notification.
        /// </summary>
        private bool disablePresetUpdateCheckNotification;

        /// <summary>
        /// The display status messages tray icon.
        /// </summary>
        private bool displayStatusMessagesTrayIcon;

        /// <summary>
        /// The enable gui tooltips.
        /// </summary>
        private bool enableGuiTooltips;

        /// <summary>
        /// The enable query editor.
        /// </summary>
        private bool enableQueryEditor;

        /// <summary>
        /// The growl after encode.
        /// </summary>
        private bool growlAfterEncode;

        /// <summary>
        /// The growl after queue.
        /// </summary>
        private bool growlAfterQueue;

        /// <summary>
        /// The log directory.
        /// </summary>
        private string logDirectory;

        /// <summary>
        /// The log verbosity options.
        /// </summary>
        private BindingList<int> logVerbosityOptions = new BindingList<int>();

        /// <summary>
        /// The min length.
        /// </summary>
        private long minLength;

        /// <summary>
        /// The minimise to tray.
        /// </summary>
        private bool minimiseToTray;

        /// <summary>
        /// The mp 4 extension options.
        /// </summary>
        private BindingList<string> mp4ExtensionOptions = new BindingList<string>();

        /// <summary>
        /// The prevent sleep.
        /// </summary>
        private bool preventSleep;

        /// <summary>
        /// The preview pictures to scan.
        /// </summary>
        private BindingList<int> previewPicturesToScan = new BindingList<int>();

        /// <summary>
        /// The priority level options.
        /// </summary>
        private BindingList<string> priorityLevelOptions = new BindingList<string>();

        /// <summary>
        /// The prompt on different query.
        /// </summary>
        private bool promptOnDifferentQuery;

        /// <summary>
        /// The remove underscores.
        /// </summary>
        private bool removeUnderscores;

        /// <summary>
        /// The selected add audio mode.
        /// </summary>
        private int selectedAddAudioMode;

        /// <summary>
        /// The selected add subtitle mode.
        /// </summary>
        private int selectedAddSubtitleMode;

        /// <summary>
        /// The selected granulairty.
        /// </summary>
        private string selectedGranulairty;

        /// <summary>
        /// The selected mp 4 extension.
        /// </summary>
        private int selectedMp4Extension;

        /// <summary>
        /// The selected preferred languages.
        /// </summary>
        private BindingList<string> preferredLanguages = new BindingList<string>();

        /// <summary>
        /// The selected preferred langauge.
        /// </summary>
        private string selectedPreferredLangauge;

        /// <summary>
        /// The selected preferred subtitle language
        /// </summary>
        private string selectedPreferredSubtitleLangauge;

        /// <summary>
        /// The selected preview count.
        /// </summary>
        private int selectedPreviewCount;

        /// <summary>
        /// The selected priority.
        /// </summary>
        private string selectedPriority;

        /// <summary>
        /// The selected verbosity.
        /// </summary>
        private int selectedVerbosity;

        /// <summary>
        /// The send file after encode.
        /// </summary>
        private bool sendFileAfterEncode;

        /// <summary>
        /// The send file to.
        /// </summary>
        private string sendFileTo;

        /// <summary>
        /// The send file to Path.
        /// </summary>
        private string sendFileToPath;


        /// <summary>
        /// The show cli window.
        /// </summary>
        private bool showCliWindow;

        /// <summary>
        /// The vlc path.
        /// </summary>
        private string vlcPath;

        /// <summary>
        /// The when done.
        /// </summary>
        private string whenDone;

        /// <summary>
        /// The when done options.
        /// </summary>
        private BindingList<string> whenDoneOptions = new BindingList<string>();

        /// <summary>
        /// Selected Langauges
        /// </summary>
        private BindingList<string> selectedLangauges = new BindingList<string>();

        /// <summary>
        /// The backing field for show advanced passthru options for Audio
        /// </summary>
        private bool showAdvancedPassthruOpts;

        #endregion

        #region Constructors and Destructors

        /// <summary>
        /// Initializes a new instance of the <see cref="OptionsViewModel"/> class.
        /// </summary>
        /// <param name="windowManager">
        /// The window manager.
        /// </param>
        /// <param name="userSettingService">
        /// The user Setting Service.
        /// </param>
        public OptionsViewModel(IWindowManager windowManager, IUserSettingService userSettingService)
        {
            this.Title = "Options";
            this.userSettingService = userSettingService;
            this.Load();
        }

        #endregion

        #region Properties

        #region General

        /// <summary>
        /// Gets or sets a value indicating whether CheckForUpdates.
        /// </summary>
        public bool CheckForUpdates
        {
            get
            {
                return this.checkForUpdates;
            }

            set
            {
                this.checkForUpdates = value;
                this.NotifyOfPropertyChange("CheckForUpdates");
            }
        }

        /// <summary>
        /// Gets or sets CheckForUpdatesFrequencies.
        /// </summary>
        public BindingList<string> CheckForUpdatesFrequencies
        {
            get
            {
                return this.checkForUpdatesFrequencies;
            }

            set
            {
                this.checkForUpdatesFrequencies = value;
                this.NotifyOfPropertyChange("CheckForUpdatesFrequencies");
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether CheckForUpdatesFrequency.
        /// </summary>
        public int CheckForUpdatesFrequency
        {
            get
            {
                return this.checkForUpdatesFrequency;
            }

            set
            {
                this.checkForUpdatesFrequency = value;
                this.NotifyOfPropertyChange("CheckForUpdatesFrequency");
            }
        }

        /// <summary>
        /// Gets or sets Arguments.
        /// </summary>
        public string Arguments
        {
            get
            {
                return this.arguments;
            }

            set
            {
                this.arguments = value;
                this.NotifyOfPropertyChange("Arguments");
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether GrowlAfterEncode.
        /// </summary>
        public bool GrowlAfterEncode
        {
            get
            {
                return this.growlAfterEncode;
            }

            set
            {
                this.growlAfterEncode = value;
                this.NotifyOfPropertyChange("GrowlAfterEncode");
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether GrowlAfterQueue.
        /// </summary>
        public bool GrowlAfterQueue
        {
            get
            {
                return this.growlAfterQueue;
            }

            set
            {
                this.growlAfterQueue = value;
                this.NotifyOfPropertyChange("GrowlAfterQueue");
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether SendFileAfterEncode.
        /// </summary>
        public bool SendFileAfterEncode
        {
            get
            {
                return this.sendFileAfterEncode;
            }

            set
            {
                this.sendFileAfterEncode = value;
                this.NotifyOfPropertyChange("SendFileAfterEncode");
            }
        }

        /// <summary>
        /// Gets or sets SendFileTo.
        /// </summary>
        public string SendFileTo
        {
            get
            {
                return this.sendFileTo;
            }

            set
            {
                this.sendFileTo = value;
                this.NotifyOfPropertyChange("SendFileTo");
            }
        }

        /// <summary>
        /// Gets or sets SendFileToPath.
        /// </summary>
        public string SendFileToPath
        {
            get
            {
                return this.sendFileToPath;
            }

            set
            {
                this.sendFileToPath = value;
                this.NotifyOfPropertyChange("SendFileToPath");
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether EnableGuiTooltips.
        /// </summary>
        public bool EnableGuiTooltips
        {
            get
            {
                return this.enableGuiTooltips;
            }

            set
            {
                this.enableGuiTooltips = value;
                this.NotifyOfPropertyChange("EnableGuiTooltips");
            }
        }

        /// <summary>
        /// Gets or sets WhenDone.
        /// </summary>
        public string WhenDone
        {
            get
            {
                return this.whenDone;
            }

            set
            {
                this.whenDone = value;
                this.NotifyOfPropertyChange("WhenDone");
            }
        }

        /// <summary>
        /// Gets or sets WhenDoneOptions.
        /// </summary>
        public BindingList<string> WhenDoneOptions
        {
            get
            {
                return this.whenDoneOptions;
            }

            set
            {
                this.whenDoneOptions = value;
                this.NotifyOfPropertyChange("WhenDoneOptions");
            }
        }
        #endregion

        #region Output Files

        /// <summary>
        /// Gets or sets AutoNameDefaultPath.
        /// </summary>
        public string AutoNameDefaultPath
        {
            get
            {
                return this.autoNameDefaultPath;
            }

            set
            {
                this.autoNameDefaultPath = value;
                this.NotifyOfPropertyChange("AutoNameDefaultPath");
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether AutomaticallyNameFiles.
        /// </summary>
        public bool AutomaticallyNameFiles
        {
            get
            {
                return this.automaticallyNameFiles;
            }

            set
            {
                this.automaticallyNameFiles = value;
                this.NotifyOfPropertyChange("AutomaticallyNameFiles");
            }
        }

        /// <summary>
        /// Gets or sets AutonameFormat.
        /// </summary>
        public string AutonameFormat
        {
            get
            {
                return this.autonameFormat;
            }

            set
            {
                this.autonameFormat = value;
                this.NotifyOfPropertyChange("AutonameFormat");
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether ChangeToTitleCase.
        /// </summary>
        public bool ChangeToTitleCase
        {
            get
            {
                return this.changeToTitleCase;
            }

            set
            {
                this.changeToTitleCase = value;
                this.NotifyOfPropertyChange("ChangeToTitleCase");
            }
        }

        /// <summary>
        /// Gets or sets Mp4ExtensionOptions.
        /// </summary>
        public BindingList<string> Mp4ExtensionOptions
        {
            get
            {
                return this.mp4ExtensionOptions;
            }

            set
            {
                this.mp4ExtensionOptions = value;
                this.NotifyOfPropertyChange("Mp4ExtensionOptions");
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether RemoveUnderscores.
        /// </summary>
        public bool RemoveUnderscores
        {
            get
            {
                return this.removeUnderscores;
            }

            set
            {
                this.removeUnderscores = value;
                this.NotifyOfPropertyChange("RemoveUnderscores");
            }
        }

        /// <summary>
        /// Gets or sets SelectedMp4Extension.
        /// </summary>
        public int SelectedMp4Extension
        {
            get
            {
                return this.selectedMp4Extension;
            }

            set
            {
                this.selectedMp4Extension = value;
                this.NotifyOfPropertyChange("SelectedMp4Extension");
            }
        }

        #endregion

        #region Preview
        /// <summary>
        /// Gets or sets VLCPath.
        /// </summary>
        public string VLCPath
        {
            get
            {
                return this.vlcPath;
            }

            set
            {
                this.vlcPath = value;
                this.NotifyOfPropertyChange("VLCPath");
            }
        }
        #endregion

        #region Audio and Subtitles

        /// <summary>
        /// Gets or sets preferredLanguages.
        /// </summary>
        public BindingList<string> PreferredLanguages
        {
            get
            {
                return this.preferredLanguages;
            }

            set
            {
                this.preferredLanguages = value;
                this.NotifyOfPropertyChange("preferredLanguages");
            }
        }

        /// <summary>
        /// Gets or sets SelectedPreferreedLangauge.
        /// </summary>
        public string SelectedPreferredLangauge
        {
            get
            {
                return this.selectedPreferredLangauge;
            }

            set
            {
                this.selectedPreferredLangauge = value;
                this.NotifyOfPropertyChange("SelectedPreferreedLangauge");
            }
        }

        /// <summary>
        /// Gets or sets SelectedPreferredSubtitleLangauge.
        /// </summary>
        public string SelectedPreferredSubtitleLangauge
        {
            get
            {
                return this.selectedPreferredSubtitleLangauge;
            }

            set
            {
                this.selectedPreferredSubtitleLangauge = value;
                this.NotifyOfPropertyChange("SelectedPreferredSubtitleLangauge");
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
        public BindingList<string> SelectedLangauges
        {
            get
            {
                return this.selectedLangauges;
            }
            set
            {
                this.selectedLangauges = value;
                this.NotifyOfPropertyChange("SelectedLangauges");
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

        /// <summary>
        /// Gets or sets AddAudioModeOptions.
        /// </summary>
        public BindingList<string> AddAudioModeOptions
        {
            get
            {
                return this.addAudioModeOptions;
            }

            set
            {
                this.addAudioModeOptions = value;
                this.NotifyOfPropertyChange("AddAudioModeOptions");
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether AddClosedCaptions.
        /// </summary>
        public bool AddClosedCaptions
        {
            get
            {
                return this.addClosedCaptions;
            }

            set
            {
                this.addClosedCaptions = value;
                this.NotifyOfPropertyChange("AddClosedCaptions");
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether AddOnlyOneAudioTrackPerLanguage.
        /// </summary>
        public bool AddOnlyOneAudioTrackPerLanguage
        {
            get
            {
                return this.addOnlyOneAudioTrackPerLanguage;
            }

            set
            {
                this.addOnlyOneAudioTrackPerLanguage = value;
                this.NotifyOfPropertyChange("AddOnlyOneAudioTrackPerLanguage");
            }
        }

        /// <summary>
        /// Gets or sets AddSubtitleModeOptions.
        /// </summary>
        public BindingList<string> AddSubtitleModeOptions
        {
            get
            {
                return this.addSubtitleModeOptions;
            }

            set
            {
                this.addSubtitleModeOptions = value;
                this.NotifyOfPropertyChange("AddSubtitleModeOptions");
            }
        }

        /// <summary>
        /// Gets or sets SelectedAddAudioMode.
        /// </summary>
        public int SelectedAddAudioMode
        {
            get
            {
                return this.selectedAddAudioMode;
            }

            set
            {
                this.selectedAddAudioMode = value;
                this.NotifyOfPropertyChange("SelectedAddAudioMode");
            }
        }

        /// <summary>
        /// Gets or sets SelectedAddSubtitleMode.
        /// </summary>
        public int SelectedAddSubtitleMode
        {
            get
            {
                return this.selectedAddSubtitleMode;
            }

            set
            {
                this.selectedAddSubtitleMode = value;
                this.NotifyOfPropertyChange("SelectedAddSubtitleMode");
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether ShowAdvancedPassthruOpts.
        /// </summary>
        public bool ShowAdvancedPassthruOpts
        {
            get
            {
                return this.showAdvancedPassthruOpts;
            }
            set
            {
                this.showAdvancedPassthruOpts = value;
                this.NotifyOfPropertyChange(() => this.ShowAdvancedPassthruOpts);
            }
        }

        #endregion

        #region System and Logging

        /// <summary>
        /// Gets or sets a value indicating whether CopyLogToEncodeDirectory.
        /// </summary>
        public bool CopyLogToEncodeDirectory
        {
            get
            {
                return this.copyLogToEncodeDirectory;
            }

            set
            {
                this.copyLogToEncodeDirectory = value;
                this.NotifyOfPropertyChange("CopyLogToEncodeDirectory");
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether CopyLogToSepcficedLocation.
        /// </summary>
        public bool CopyLogToSepcficedLocation
        {
            get
            {
                return this.copyLogToSepcficedLocation;
            }

            set
            {
                this.copyLogToSepcficedLocation = value;
                this.NotifyOfPropertyChange("CopyLogToSepcficedLocation");
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether ClearOldOlgs.
        /// </summary>
        public bool ClearOldOlgs
        {
            get
            {
                return this.clearOldOlgs;
            }

            set
            {
                this.clearOldOlgs = value;
                this.NotifyOfPropertyChange("ClearOldOlgs");
            }
        }

        /// <summary>
        /// Gets or sets LogDirectory.
        /// </summary>
        public string LogDirectory
        {
            get
            {
                return this.logDirectory;
            }

            set
            {
                this.logDirectory = value;
                this.NotifyOfPropertyChange("LogDirectory");
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether PreventSleep.
        /// </summary>
        public bool PreventSleep
        {
            get
            {
                return this.preventSleep;
            }

            set
            {
                this.preventSleep = value;
                this.NotifyOfPropertyChange("PreventSleep");
            }
        }

        /// <summary>
        /// Gets or sets PriorityLevelOptions.
        /// </summary>
        public BindingList<string> PriorityLevelOptions
        {
            get
            {
                return this.priorityLevelOptions;
            }

            set
            {
                this.priorityLevelOptions = value;
                this.NotifyOfPropertyChange("PriorityLevelOptions");
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether SelectedGranulairty.
        /// </summary>
        public string SelectedGranulairty
        {
            get
            {
                return this.selectedGranulairty;
            }

            set
            {
                this.selectedGranulairty = value;
                this.NotifyOfPropertyChange("SelectedGranulairty");
            }
        }

        /// <summary>
        /// Gets or sets SelectedPriority.
        /// </summary>
        public string SelectedPriority
        {
            get
            {
                return this.selectedPriority;
            }

            set
            {
                this.selectedPriority = value;
                this.NotifyOfPropertyChange("SelectedPriority");
            }
        }
        #endregion

        #region Advanced

        /// <summary>
        /// Gets or sets a value indicating whether EnableQueryEditor.
        /// </summary>
        public bool EnableQueryEditor
        {
            get
            {
                return this.enableQueryEditor;
            }

            set
            {
                this.enableQueryEditor = value;
                this.NotifyOfPropertyChange("EnableQueryEditor");
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether PromptOnDifferentQuery.
        /// </summary>
        public bool PromptOnDifferentQuery
        {
            get
            {
                return this.promptOnDifferentQuery;
            }

            set
            {
                this.promptOnDifferentQuery = value;
                this.NotifyOfPropertyChange("PromptOnDifferentQuery");
            }
        }

        /// <summary>
        /// Gets or sets ConstantQualityGranularity.
        /// </summary>
        public BindingList<string> ConstantQualityGranularity
        {
            get
            {
                return this.constantQualityGranularity;
            }

            set
            {
                this.constantQualityGranularity = value;
                this.NotifyOfPropertyChange("ConstantQualityGranularity");
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether DisableLibdvdNav.
        /// </summary>
        public bool DisableLibdvdNav
        {
            get
            {
                return this.disableLibdvdNav;
            }

            set
            {
                this.disableLibdvdNav = value;
                this.NotifyOfPropertyChange("DisableLibdvdNav");
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether disablePresetUpdateCheckNotification.
        /// </summary>
        public bool DisablePresetUpdateCheckNotification
        {
            get
            {
                return this.disablePresetUpdateCheckNotification;
            }

            set
            {
                this.disablePresetUpdateCheckNotification = value;
                this.NotifyOfPropertyChange("DisablePresetUpdateCheckNotification");
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether DisplayStatusMessagesTrayIcon.
        /// </summary>
        public bool DisplayStatusMessagesTrayIcon
        {
            get
            {
                return this.displayStatusMessagesTrayIcon;
            }

            set
            {
                this.displayStatusMessagesTrayIcon = value;
                this.NotifyOfPropertyChange("DisplayStatusMessagesTrayIcon");
            }
        }

        /// <summary>
        /// Gets or sets LogVerbosityOptions.
        /// </summary>
        public BindingList<int> LogVerbosityOptions
        {
            get
            {
                return this.logVerbosityOptions;
            }

            set
            {
                this.logVerbosityOptions = value;
                this.NotifyOfPropertyChange("LogVerbosityOptions");
            }
        }

        /// <summary>
        /// Gets or sets MinLength.
        /// </summary>
        public long MinLength
        {
            get
            {
                return this.minLength;
            }

            set
            {
                this.minLength = value;
                this.NotifyOfPropertyChange("MinLength");
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether MinimiseToTray.
        /// </summary>
        public bool MinimiseToTray
        {
            get
            {
                return this.minimiseToTray;
            }

            set
            {
                this.minimiseToTray = value;
                this.NotifyOfPropertyChange("MinimiseToTray");
            }
        }

        /// <summary>
        /// Gets or sets PreviewPicturesToScan.
        /// </summary>
        public BindingList<int> PreviewPicturesToScan
        {
            get
            {
                return this.previewPicturesToScan;
            }

            set
            {
                this.previewPicturesToScan = value;
                this.NotifyOfPropertyChange("PreviewPicturesToScan");
            }
        }

        /// <summary>
        /// Gets or sets SelectedPreviewCount.
        /// </summary>
        public int SelectedPreviewCount
        {
            get
            {
                return this.selectedPreviewCount;
            }

            set
            {
                this.selectedPreviewCount = value;
                this.NotifyOfPropertyChange("SelectedPreviewCount");
            }
        }

        /// <summary>
        /// Gets or sets SelectedVerbosity.
        /// </summary>
        public int SelectedVerbosity
        {
            get
            {
                return this.selectedVerbosity;
            }

            set
            {
                this.selectedVerbosity = value;
                this.NotifyOfPropertyChange("SelectedVerbosity");
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether ShowCliWindow.
        /// </summary>
        public bool ShowCliWindow
        {
            get
            {
                return this.showCliWindow;
            }

            set
            {
                this.showCliWindow = value;
                this.NotifyOfPropertyChange("ShowCliWindow");
            }
        }
        #endregion

        #endregion

        #region Public Methods

        /// <summary>
        /// Load User Settings
        /// </summary>
        public override void OnLoad()
        {
            // #############################
            // General
            // #############################

            this.enableGuiTooltips = this.userSettingService.GetUserSetting<bool>(UserSettingConstants.TooltipEnable);
            this.checkForUpdates = this.userSettingService.GetUserSetting<bool>(UserSettingConstants.UpdateStatus);

            // Days between update checks
            this.checkForUpdatesFrequencies.Add("Daily");
            this.checkForUpdatesFrequencies.Add("Weekly");
            this.checkForUpdatesFrequencies.Add("Monthly");

            // TODO Refactor this.
            switch (this.userSettingService.GetUserSetting<int>(UserSettingConstants.DaysBetweenUpdateCheck))
            {
                case 1:
                    this.checkForUpdatesFrequency = 1;
                    break;
                case 7:
                    this.checkForUpdatesFrequency = 2;
                    break;
                case 30:
                    this.checkForUpdatesFrequency = 3;
                    break;
            }

            // On Encode Completeion Action
            this.whenDoneOptions.Add("Do nothing");
            this.whenDoneOptions.Add("Shutdown");
            this.whenDoneOptions.Add("Suspend");
            this.whenDoneOptions.Add("Hibernate");
            this.whenDoneOptions.Add("Lock system");
            this.whenDoneOptions.Add("Log off");
            this.whenDoneOptions.Add("Quit HandBrake");
            this.whenDone = userSettingService.GetUserSetting<string>("WhenCompleteAction");

            this.growlAfterEncode = userSettingService.GetUserSetting<bool>(ASUserSettingConstants.GrowlEncode);
            this.growlAfterQueue = userSettingService.GetUserSetting<bool>(ASUserSettingConstants.GrowlQueue);
            this.sendFileAfterEncode = this.userSettingService.GetUserSetting<bool>(ASUserSettingConstants.SendFile);
            this.sendFileTo = Path.GetFileNameWithoutExtension(this.userSettingService.GetUserSetting<string>(ASUserSettingConstants.SendFileTo)) ?? string.Empty;
            this.sendFileToPath = this.userSettingService.GetUserSetting<string>(ASUserSettingConstants.SendFileTo) ?? string.Empty;
            this.arguments = this.userSettingService.GetUserSetting<string>(ASUserSettingConstants.SendFileToArgs) ?? string.Empty;

            // #############################
            // Output Settings
            // #############################

            // Enable auto naming feature.)
            this.automaticallyNameFiles = this.userSettingService.GetUserSetting<bool>(UserSettingConstants.AutoNaming);

            // Store the auto name path
            this.autoNameDefaultPath = this.userSettingService.GetUserSetting<string>(UserSettingConstants.AutoNamePath) ?? string.Empty;
            if (string.IsNullOrEmpty(this.autoNameDefaultPath))
                this.autoNameDefaultPath = "Click 'Browse' to set the default location";

            // Store auto name format
            this.autonameFormat = this.userSettingService.GetUserSetting<string>(UserSettingConstants.AutoNameFormat) ?? string.Empty;

            // Use iPod/iTunes friendly .m4v extension for MP4 files.
            this.mp4ExtensionOptions.Add("Automatic");
            this.mp4ExtensionOptions.Add("Always use MP4");
            this.mp4ExtensionOptions.Add("Always use M4V");
            this.selectedMp4Extension = this.userSettingService.GetUserSetting<int>(UserSettingConstants.UseM4v);

            // Remove Underscores
            this.removeUnderscores = this.userSettingService.GetUserSetting<bool>(UserSettingConstants.AutoNameRemoveUnderscore);

            // Title case
            this.changeToTitleCase = this.userSettingService.GetUserSetting<bool>(UserSettingConstants.AutoNameTitleCase);

            // #############################
            // Picture Tab
            // #############################

            // VLC Path
            this.vlcPath = this.userSettingService.GetUserSetting<string>(UserSettingConstants.VLC_Path) ?? string.Empty;

            // #############################
            // Audio and Subtitles Tab
            // #############################

            this.SelectedAvailableToMove = new BindingList<string>();
            this.SelectedLangaugesToMove = new BindingList<string>();

            IDictionary<string, string> langList = LanguageUtilities.MapLanguages();

            foreach (string selectedItem in this.userSettingService.GetUserSetting<StringCollection>(UserSettingConstants.SelectedLanguages))
            {
                // removing wrong keys when a new Language list comes out.
                if (langList.ContainsKey(selectedItem))
                {
                    this.selectedLangauges.Add(selectedItem);
                }
            }

            foreach (string item in langList.Keys)
            {
                this.preferredLanguages.Add(item);

                // In the available languages should be no "Any" and no selected language.
                if ((item != "Any") && (!this.userSettingService.GetUserSetting<StringCollection>(UserSettingConstants.SelectedLanguages).Contains(item)))
                {
                    this.availableLanguages.Add(item);
                }
            }

            this.selectedPreferredLangauge = this.userSettingService.GetUserSetting<string>(UserSettingConstants.NativeLanguage) ?? string.Empty;
            this.selectedPreferredSubtitleLangauge = this.userSettingService.GetUserSetting<string>(UserSettingConstants.NativeLanguageForSubtitles) ?? string.Empty;

            this.AddAudioModeOptions.Add("None");
            this.AddAudioModeOptions.Add("All Remaining Tracks");
            this.AddAudioModeOptions.Add("All for Selected Languages");

            this.AddSubtitleModeOptions.Add("None");
            this.AddSubtitleModeOptions.Add("All");
            this.AddSubtitleModeOptions.Add("First");
            this.AddSubtitleModeOptions.Add("Selected");
            this.AddSubtitleModeOptions.Add("Prefered Only (First)");
            this.AddSubtitleModeOptions.Add("Prefered Only (All)");

            this.selectedAddAudioMode = this.userSettingService.GetUserSetting<int>(UserSettingConstants.DubModeAudio);
            this.selectedAddSubtitleMode = this.userSettingService.GetUserSetting<int>(UserSettingConstants.DubModeSubtitle);

            this.addOnlyOneAudioTrackPerLanguage = this.userSettingService.GetUserSetting<bool>(UserSettingConstants.AddOnlyOneAudioPerLanguage);

            this.addClosedCaptions = this.userSettingService.GetUserSetting<bool>(UserSettingConstants.UseClosedCaption);
            this.showAdvancedPassthruOpts = this.userSettingService.GetUserSetting<bool>(UserSettingConstants.ShowAdvancedAudioPassthruOpts);

            // #############################
            // CLI
            // #############################

            // Priority level for encodes
            this.priorityLevelOptions.Add("Realtime");
            this.priorityLevelOptions.Add("High");
            this.priorityLevelOptions.Add("Above Normal");
            this.priorityLevelOptions.Add("Normal");
            this.priorityLevelOptions.Add("Below Normal");
            this.priorityLevelOptions.Add("Low");
            this.selectedPriority = userSettingService.GetUserSetting<string>(ASUserSettingConstants.ProcessPriority);

            this.preventSleep = userSettingService.GetUserSetting<bool>(ASUserSettingConstants.PreventSleep);

            // Log Verbosity Level
            this.logVerbosityOptions.Add(0);
            this.logVerbosityOptions.Add(1);
            this.logVerbosityOptions.Add(2);
            this.selectedVerbosity = userSettingService.GetUserSetting<int>(ASUserSettingConstants.Verbosity);

            // Logs
            this.copyLogToEncodeDirectory = userSettingService.GetUserSetting<bool>(ASUserSettingConstants.SaveLogWithVideo);
            this.copyLogToSepcficedLocation = userSettingService.GetUserSetting<bool>(ASUserSettingConstants.SaveLogToCopyDirectory);

            // The saved log path
            this.logDirectory = userSettingService.GetUserSetting<string>(ASUserSettingConstants.SaveLogCopyDirectory) ?? string.Empty;

            this.clearOldOlgs = this.userSettingService.GetUserSetting<bool>(UserSettingConstants.ClearOldLogs);

            // #############################
            // Advanced
            // #############################

            // Minimise to Tray
            this.displayStatusMessagesTrayIcon = this.userSettingService.GetUserSetting<bool>(UserSettingConstants.TrayIconAlerts);
            this.minimiseToTray = this.userSettingService.GetUserSetting<bool>(UserSettingConstants.MainWindowMinimize);
            this.enableQueryEditor = this.userSettingService.GetUserSetting<bool>(UserSettingConstants.QueryEditorTab);
            this.promptOnDifferentQuery = this.userSettingService.GetUserSetting<bool>(UserSettingConstants.PromptOnUnmatchingQueries);
            this.disablePresetUpdateCheckNotification = this.userSettingService.GetUserSetting<bool>(UserSettingConstants.PresetNotification);
            this.showCliWindow = userSettingService.GetUserSetting<bool>(ASUserSettingConstants.ShowCLI);

            // Set the preview count
            this.PreviewPicturesToScan.Add(10);
            this.PreviewPicturesToScan.Add(15);
            this.PreviewPicturesToScan.Add(20);
            this.PreviewPicturesToScan.Add(25);
            this.PreviewPicturesToScan.Add(30);
            this.selectedPreviewCount = this.userSettingService.GetUserSetting<int>(ASUserSettingConstants.PreviewScanCount);

            // x264 step
            this.ConstantQualityGranularity.Add("1.0");
            this.ConstantQualityGranularity.Add("0.50");
            this.ConstantQualityGranularity.Add("0.25");
            this.ConstantQualityGranularity.Add("0.20");
            this.selectedGranulairty = userSettingService.GetUserSetting<double>(ASUserSettingConstants.X264Step).ToString("0.00", CultureInfo.InvariantCulture);

            // Min Title Length
            this.minLength = this.userSettingService.GetUserSetting<int>(ASUserSettingConstants.MinScanDuration);

            // Use Experimental dvdnav
            this.disableLibdvdNav = userSettingService.GetUserSetting<bool>(ASUserSettingConstants.DisableLibDvdNav);
        }

        /// <summary>
        /// Close this window.
        /// </summary>
        public void Close()
        {
            this.Save();
            this.TryClose();
        }

        /// <summary>
        /// Browse - Send File To
        /// </summary>
        public void BrowseSendFileTo()
        {
            VistaOpenFileDialog dialog = new VistaOpenFileDialog { Filter = "All files (*.*)|*.*" };
            dialog.ShowDialog();
            this.SendFileTo = Path.GetFileNameWithoutExtension(dialog.FileName);
            this.sendFileToPath = dialog.FileName;
        }

        /// <summary>
        /// Browse Auto Name Path
        /// </summary>
        public void BrowseAutoNamePath()
        {
            VistaFolderBrowserDialog dialog = new VistaFolderBrowserDialog { Description = "Please select a folder.", UseDescriptionForTitle = true };
            dialog.ShowDialog();
            this.AutoNameDefaultPath = dialog.SelectedPath;
        }

        /// <summary>
        /// Browse VLC Path
        /// </summary>
        public void BrowseVlcPath()
        {
            VistaOpenFileDialog dialog = new VistaOpenFileDialog { Filter = "All files (*.exe)|*.exe" };
            dialog.ShowDialog();
            this.VLCPath = dialog.FileName;
        }

        /// <summary>
        /// Audio List Move Left
        /// </summary>
        public void LanguageMoveLeft()
        {
            if (this.AvailableLanguages.Count > 0)
            {
                List<string> copiedList = SelectedAvailableToMove.ToList();
                foreach (string item in copiedList)
                {
                    this.AvailableLanguages.Remove(item);
                    this.SelectedLangauges.Add(item);
                }
            }
        }

        /// <summary>
        /// Audio List Move Right
        /// </summary>
        public void LanguageMoveRight()
        {
            if (this.SelectedLangauges.Count > 0)
            {
                List<string> copiedList = SelectedLangaugesToMove.ToList();
                foreach (string item in copiedList)
                {
                    this.SelectedLangauges.Remove(item);
                    this.AvailableLanguages.Add(item);
                }
            }
        }

        /// <summary>
        /// Audio List Clear all selected languages
        /// </summary>
        public void LanguageClearAll()
        {
            foreach (string item in this.SelectedLangauges)
            {
                this.AvailableLanguages.Add(item);
            }

            this.SelectedLangauges.Clear();
        }

        /// <summary>
        ///  Audio List Language Move UP
        /// </summary>
        public void LanguageMoveUp()
        {
            List<string> langauges = this.SelectedLangauges.ToList();
            foreach (string item in langauges)
            {
                if (this.SelectedLangaugesToMove.Contains(item))
                {
                    int index = this.SelectedLangauges.IndexOf(item);
                    if (index != 0)
                    {
                        this.SelectedLangauges.Remove(item);
                        this.SelectedLangauges.Insert(index - 1, item);
                    }
                }
            }
        }

        /// <summary>
        /// Audio List Language Move Down
        /// </summary>
        public void LanguageMoveDown()
        {
            List<string> langauges = this.SelectedLangauges.ToList();
            int count = this.SelectedLangauges.Count;
            foreach (string item in langauges)
            {
                if (this.SelectedLangaugesToMove.Contains(item))
                {
                    int index = this.SelectedLangauges.IndexOf(item);
                    if ((index + 1) != count)
                    {
                        this.SelectedLangauges.Remove(item);
                        this.SelectedLangauges.Insert(index + 1, item);
                    }
                }
            }
        }

        /// <summary>
        /// Browse - Log Path
        /// </summary>
        public void BrowseLogPath()
        {
            VistaFolderBrowserDialog dialog = new VistaFolderBrowserDialog { Description = "Please select a folder.", UseDescriptionForTitle = true };
            dialog.ShowDialog();
            this.LogDirectory = dialog.SelectedPath;
        }

        /// <summary>
        /// View the Default Log Directory for HandBrake
        /// </summary>
        public void ViewLogDirectory()
        {
            string logDir = Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData) + "\\HandBrake\\logs";
            string windir = Environment.GetEnvironmentVariable("WINDIR");
            Process prc = new Process { StartInfo = { FileName = windir + @"\explorer.exe", Arguments = logDir } };
            prc.Start();
        }

        /// <summary>
        /// Clear HandBrakes log directory.
        /// </summary>
        public void ClearLogHistory()
        {
            MessageBoxResult result = MessageBox.Show("Are you sure you wish to clear the log file directory?", "Clear Logs",
                                                  MessageBoxButton.YesNoCancel, MessageBoxImage.Question);
            if (result == MessageBoxResult.Yes)
            {
                GeneralUtilities.ClearLogFiles(0);
                MessageBox.Show("HandBrake's Log file directory has been cleared!", "Notice", MessageBoxButton.OK, MessageBoxImage.Information);
            }
        }

        #endregion

        /// <summary>
        /// Save the settings selected
        /// </summary>
        private void Save()
        {
            /* General */
            this.userSettingService.SetUserSetting(UserSettingConstants.UpdateStatus, this.CheckForUpdates);
            this.userSettingService.SetUserSetting(UserSettingConstants.DaysBetweenUpdateCheck, this.CheckForUpdatesFrequency);
            this.userSettingService.SetUserSetting(UserSettingConstants.TooltipEnable, this.EnableGuiTooltips);
            this.userSettingService.SetUserSetting(ASUserSettingConstants.WhenCompleteAction, this.WhenDone);
            this.userSettingService.SetUserSetting(ASUserSettingConstants.GrowlQueue, this.GrowlAfterQueue);
            this.userSettingService.SetUserSetting(ASUserSettingConstants.GrowlEncode, this.GrowlAfterEncode);
            this.userSettingService.SetUserSetting(ASUserSettingConstants.SendFileTo, this.SendFileToPath);
            this.userSettingService.SetUserSetting(ASUserSettingConstants.SendFile, this.SendFileAfterEncode);
            this.userSettingService.SetUserSetting(ASUserSettingConstants.SendFileToArgs, this.Arguments);

            /* Output Files */
            this.userSettingService.SetUserSetting(UserSettingConstants.AutoNaming, this.AutomaticallyNameFiles);
            this.userSettingService.SetUserSetting(UserSettingConstants.AutoNameFormat, this.AutonameFormat);
            this.userSettingService.SetUserSetting(UserSettingConstants.AutoNamePath, this.AutoNameDefaultPath);
            this.userSettingService.SetUserSetting(UserSettingConstants.UseM4v, this.SelectedMp4Extension);
            this.userSettingService.SetUserSetting(UserSettingConstants.AutoNameRemoveUnderscore, this.RemoveUnderscores);
            this.userSettingService.SetUserSetting(UserSettingConstants.AutoNameTitleCase, this.ChangeToTitleCase);

            /* Previews */
            this.userSettingService.SetUserSetting(UserSettingConstants.VLC_Path, this.VLCPath);

            /* Audio and Subtitles */
            this.userSettingService.SetUserSetting(UserSettingConstants.NativeLanguage, this.SelectedPreferredLangauge);
            this.userSettingService.SetUserSetting(UserSettingConstants.NativeLanguageForSubtitles, this.SelectedPreferredSubtitleLangauge);
            StringCollection collection = new StringCollection();
            collection.AddRange(this.SelectedLangauges.ToArray());
            this.userSettingService.SetUserSetting(UserSettingConstants.SelectedLanguages, collection);
            this.userSettingService.SetUserSetting(UserSettingConstants.AddOnlyOneAudioPerLanguage, this.AddOnlyOneAudioTrackPerLanguage);
            this.userSettingService.SetUserSetting(UserSettingConstants.UseClosedCaption, this.AddClosedCaptions);
            this.userSettingService.SetUserSetting(UserSettingConstants.DubModeAudio, this.SelectedAddAudioMode);
            this.userSettingService.SetUserSetting(UserSettingConstants.DubModeSubtitle, this.SelectedAddSubtitleMode);
            this.userSettingService.SetUserSetting(UserSettingConstants.ShowAdvancedAudioPassthruOpts, this.ShowAdvancedPassthruOpts);

            /* System and Logging */
            userSettingService.SetUserSetting(ASUserSettingConstants.ProcessPriority, this.SelectedPriority);
            userSettingService.SetUserSetting(ASUserSettingConstants.PreventSleep, this.PreventSleep);
            userSettingService.SetUserSetting(ASUserSettingConstants.Verbosity, this.SelectedVerbosity);
            userSettingService.SetUserSetting(ASUserSettingConstants.SaveLogWithVideo, this.CopyLogToEncodeDirectory);
            userSettingService.SetUserSetting(ASUserSettingConstants.SaveLogToCopyDirectory, this.CopyLogToSepcficedLocation);
            userSettingService.SetUserSetting(ASUserSettingConstants.SaveLogCopyDirectory, this.LogDirectory);
            userSettingService.SetUserSetting(UserSettingConstants.ClearOldLogs, this.ClearOldOlgs);

            /* Advanced */
            userSettingService.SetUserSetting(UserSettingConstants.MainWindowMinimize, this.MinimiseToTray);
            userSettingService.SetUserSetting(UserSettingConstants.TrayIconAlerts, this.DisplayStatusMessagesTrayIcon);
            userSettingService.SetUserSetting(UserSettingConstants.QueryEditorTab, this.EnableQueryEditor);
            userSettingService.SetUserSetting(UserSettingConstants.PromptOnUnmatchingQueries, this.PromptOnDifferentQuery);
            userSettingService.SetUserSetting(UserSettingConstants.PresetNotification, this.DisablePresetUpdateCheckNotification);
            userSettingService.SetUserSetting(ASUserSettingConstants.ShowCLI, this.ShowCliWindow);
            userSettingService.SetUserSetting(ASUserSettingConstants.PreviewScanCount, this.SelectedPreviewCount);
            userSettingService.SetUserSetting(ASUserSettingConstants.X264Step, double.Parse(this.SelectedGranulairty));

            int value;
            if (int.TryParse(this.MinLength.ToString(), out value))
            {
                this.userSettingService.SetUserSetting(ASUserSettingConstants.MinScanDuration, value);
            }

            userSettingService.SetUserSetting(ASUserSettingConstants.DisableLibDvdNav, this.DisableLibdvdNav);
        }
    }
}