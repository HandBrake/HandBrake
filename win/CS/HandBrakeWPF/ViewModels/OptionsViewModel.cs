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
    using System.ComponentModel;
    using System.ComponentModel.Composition;

    using Caliburn.Micro;

    using HandBrakeWPF.ViewModels.Interfaces;

    /// <summary>
    /// The Options View Model
    /// </summary>
    [Export(typeof(IOptionsViewModel))]
    public class OptionsViewModel : ViewModelBase, IOptionsViewModel
    {
        #region Constants and Fields

        /// <summary>
        /// The add audio mode options.
        /// </summary>
        private BindingList<string> addAudioModeOptions;

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
        private BindingList<string> addSubtitleModeOptions;

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
        private BindingList<string> availableLanguages;

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
        private BindingList<string> checkForUpdatesFrequencies;

        /// <summary>
        /// The check for updates frequency.
        /// </summary>
        private bool checkForUpdatesFrequency;

        /// <summary>
        /// The clear old olgs.
        /// </summary>
        private bool clearOldOlgs;

        /// <summary>
        /// The constant quality granularity.
        /// </summary>
        private BindingList<string> constantQualityGranularity;

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
        private bool disablePResetUpdateCheckNotification;

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
        private BindingList<string> logVerbosityOptions;

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
        private BindingList<string> mp4ExtensionOptions;

        /// <summary>
        /// The prevent sleep.
        /// </summary>
        private bool preventSleep;

        /// <summary>
        /// The preview pictures to scan.
        /// </summary>
        private BindingList<int> previewPicturesToScan;

        /// <summary>
        /// The priority level options.
        /// </summary>
        private BindingList<string> priorityLevelOptions;

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
        private string selectedAddAudioMode;

        /// <summary>
        /// The selected add subtitle mode.
        /// </summary>
        private string selectedAddSubtitleMode;

        /// <summary>
        /// The selected granulairty.
        /// </summary>
        private bool selectedGranulairty;

        /// <summary>
        /// The selected mp 4 extension.
        /// </summary>
        private string selectedMp4Extension;

        /// <summary>
        /// The selected preferred languages.
        /// </summary>
        private BindingList<string> selectedPreferredLanguages;

        /// <summary>
        /// The selected preferreed langauge.
        /// </summary>
        private string selectedPreferreedLangauge;

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
        private string selectedVerbosity;

        /// <summary>
        /// The send file after encode.
        /// </summary>
        private bool sendFileAfterEncode;

        /// <summary>
        /// The send file to.
        /// </summary>
        private string sendFileTo;

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
        private BindingList<string> whenDoneOptions;

        #endregion

        #region Constructors and Destructors

        /// <summary>
        /// Initializes a new instance of the <see cref="OptionsViewModel"/> class.
        /// </summary>
        /// <param name="windowManager">
        /// The window manager.
        /// </param>
        public OptionsViewModel(IWindowManager windowManager)
            : base(windowManager)
        {
        }

        #endregion

        #region Properties
        /* General */

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

        /* Output Files */

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
        public bool CheckForUpdatesFrequency
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
        /// Gets or sets a value indicating whether DisablePResetUpdateCheckNotification.
        /// </summary>
        public bool DisablePResetUpdateCheckNotification
        {
            get
            {
                return this.disablePResetUpdateCheckNotification;
            }

            set
            {
                this.disablePResetUpdateCheckNotification = value;
                this.NotifyOfPropertyChange("DisablePResetUpdateCheckNotification");
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
        /// Gets or sets LogVerbosityOptions.
        /// </summary>
        public BindingList<string> LogVerbosityOptions
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
        /// Gets or sets SelectedAddAudioMode.
        /// </summary>
        public string SelectedAddAudioMode
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
        public string SelectedAddSubtitleMode
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
        /// Gets or sets a value indicating whether SelectedGranulairty.
        /// </summary>
        public bool SelectedGranulairty
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
        /// Gets or sets SelectedMp4Extension.
        /// </summary>
        public string SelectedMp4Extension
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

        /// <summary>
        /// Gets or sets SelectedPreferredLanguages.
        /// </summary>
        public BindingList<string> SelectedPreferredLanguages
        {
            get
            {
                return this.selectedPreferredLanguages;
            }

            set
            {
                this.selectedPreferredLanguages = value;
                this.NotifyOfPropertyChange("SelectedPreferredLanguages");
            }
        }

        /// <summary>
        /// Gets or sets SelectedPreferreedLangauge.
        /// </summary>
        public string SelectedPreferreedLangauge
        {
            get
            {
                return this.selectedPreferreedLangauge;
            }

            set
            {
                this.selectedPreferreedLangauge = value;
                this.NotifyOfPropertyChange("SelectedPreferreedLangauge");
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

        /// <summary>
        /// Gets or sets SelectedVerbosity.
        /// </summary>
        public string SelectedVerbosity
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

        #region Public Methods

        /// <summary>
        /// Close this window.
        /// </summary>
        public void Close()
        {
            this.TryClose();
        }

        #endregion
    }
}