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
    using System.Collections.Generic;
    using System.Collections.Specialized;
    using System.ComponentModel;
    using System.ComponentModel.Composition;
    using System.Globalization;
    using System.IO;

    using Caliburn.Micro;

    using HandBrake.ApplicationServices;
    using HandBrake.ApplicationServices.Services.Interfaces;
    using HandBrake.ApplicationServices.Utilities;

    using HandBrakeWPF.ViewModels.Interfaces;

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
        /// A Property to block Saving while the screen is loading.
        /// </summary>
        private bool isLoading = true;

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
        private BindingList<string> selectedPreferredLanguages = new BindingList<string>();

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
            : base(windowManager)
        {
            this.userSettingService = userSettingService;
            this.Load();
        }

        #endregion

        #region Properties

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
        /// Load User Settings
        /// </summary>
        public void Load()
        {
            // #############################
            // Screen Setup
            // #############################

            IDictionary<string, string> langList = LanguageUtilities.MapLanguages();

            foreach (string selectedItem in this.userSettingService.GetUserSetting<StringCollection>(UserSettingConstants.SelectedLanguages))
            {
                // removing wrong keys when a new Language list comes out.
                if (langList.ContainsKey(selectedItem))
                {
                    this.selectedPreferredLanguages.Add(selectedItem);
                }
            }

            foreach (string item in langList.Keys)
            {
                this.selectedPreferredLanguages.Add(item);

                // In the available languages should be no "Any" and no selected language.
                if ((item != "Any") && (!this.userSettingService.GetUserSetting<StringCollection>(UserSettingConstants.SelectedLanguages).Contains(item)))
                {
                    this.availableLanguages.Add(item);
                }
            }

            // #############################
            // General
            // #############################

            // Enable Tooltips.
            if (this.userSettingService.GetUserSetting<bool>(UserSettingConstants.TooltipEnable))
            {
                this.enableGuiTooltips = true;
            }

            // Update Check
            if (this.userSettingService.GetUserSetting<bool>(UserSettingConstants.UpdateStatus))
            {
                this.checkForUpdates = true;
            }
   
            // Days between update checks
            this.checkForUpdatesFrequency =
                        this.userSettingService.GetUserSetting<int>(UserSettingConstants.DaysBetweenUpdateCheck);

            // On Encode Completeion Action
            this.whenDoneOptions.Add("Do nothing");
            this.whenDoneOptions.Add("Shutdown");
            this.whenDoneOptions.Add("Suspend");
            this.whenDoneOptions.Add("Hibernate");
            this.whenDoneOptions.Add("Lock system");
            this.whenDoneOptions.Add("Log off");
            this.whenDoneOptions.Add("Quit HandBrake");
            this.whenDone = userSettingService.GetUserSetting<string>("WhenCompleteAction");
            
            // Growl.
            if (userSettingService.GetUserSetting<bool>(ASUserSettingConstants.GrowlEncode))
            {
                this.growlAfterEncode = true;
            }

            if (userSettingService.GetUserSetting<bool>(ASUserSettingConstants.GrowlQueue))
            {
                this.growlAfterQueue = true;
            }

            this.SendFileAfterEncode = this.userSettingService.GetUserSetting<bool>(ASUserSettingConstants.SendFile);
            this.sendFileTo = Path.GetFileNameWithoutExtension(this.userSettingService.GetUserSetting<string>(ASUserSettingConstants.SendFileTo));
            this.arguments = this.userSettingService.GetUserSetting<string>(ASUserSettingConstants.SendFileToArgs);

            // #############################
            // Output Settings
            // #############################

            // Enable auto naming feature.)
            if (this.userSettingService.GetUserSetting<bool>(UserSettingConstants.AutoNaming))
            {
                this.AutomaticallyNameFiles = true;
            }

            // Store the auto name path
            this.autoNameDefaultPath = this.userSettingService.GetUserSetting<string>(UserSettingConstants.AutoNamePath);
            if (string.IsNullOrEmpty(this.autoNameDefaultPath))
                this.autoNameDefaultPath = "Click 'Browse' to set the default location";

            // Store auto name format
            this.autonameFormat = this.userSettingService.GetUserSetting<string>(UserSettingConstants.AutoNameFormat);

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
            this.vlcPath = this.userSettingService.GetUserSetting<string>(UserSettingConstants.VLC_Path);

            // #############################
            // Audio and Subtitles Tab
            // #############################

            this.selectedPreferreedLangauge = this.userSettingService.GetUserSetting<string>(UserSettingConstants.NativeLanguage);

            this.AddAudioModeOptions.Add("None");
            this.AddAudioModeOptions.Add("All Remaining Tracks");
            this.AddAudioModeOptions.Add("All for Selected Languages");

            this.AddSubtitleModeOptions.Add("None");
            this.AddSubtitleModeOptions.Add("All");
            this.AddSubtitleModeOptions.Add("First");
            this.AddSubtitleModeOptions.Add("Selected");
            this.AddSubtitleModeOptions.Add("Preferred Only");

            this.selectedAddAudioMode = this.userSettingService.GetUserSetting<int>(UserSettingConstants.DubModeAudio);
            this.selectedAddSubtitleMode = this.userSettingService.GetUserSetting<int>(UserSettingConstants.DubModeSubtitle);

            this.addOnlyOneAudioTrackPerLanguage = this.userSettingService.GetUserSetting<bool>(UserSettingConstants.AddOnlyOneAudioPerLanguage);

            this.addClosedCaptions = this.userSettingService.GetUserSetting<bool>(UserSettingConstants.UseClosedCaption);

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

            // Save logs in the same directory as encoded files
            if (userSettingService.GetUserSetting<bool>(ASUserSettingConstants.SaveLogWithVideo))
            {
                this.copyLogToEncodeDirectory = true;
            }

            // Save Logs in a specified path
            if (userSettingService.GetUserSetting<bool>(ASUserSettingConstants.SaveLogToCopyDirectory))
            {
                this.copyLogToSepcficedLocation = true;
            }

            // The saved log path
            this.logDirectory = userSettingService.GetUserSetting<string>(ASUserSettingConstants.SaveLogCopyDirectory);

            this.clearOldOlgs = this.userSettingService.GetUserSetting<bool>(UserSettingConstants.ClearOldLogs);

            // #############################
            // Advanced
            // #############################

            // Minimise to Tray
            if (this.userSettingService.GetUserSetting<bool>(UserSettingConstants.TrayIconAlerts))
            {
                this.DisplayStatusMessagesTrayIcon = true;
            }

            // Tray Balloon popups
            if (this.userSettingService.GetUserSetting<bool>(UserSettingConstants.MainWindowMinimize))
            {
                this.minimiseToTray = true;
            }

            // Enable / Disable Query editor tab
            if (this.userSettingService.GetUserSetting<bool>(UserSettingConstants.QueryEditorTab))
            {
                this.enableQueryEditor = true;
            }
            
            // Prompt on inconsistant queries
            this.promptOnDifferentQuery = this.userSettingService.GetUserSetting<bool>(UserSettingConstants.PromptOnUnmatchingQueries);

            // Preset update notification
            if (this.userSettingService.GetUserSetting<bool>(UserSettingConstants.PresetNotification))
            {
                this.disablePresetUpdateCheckNotification = true;
            }

            // Show CLI Window
            this.showCliWindow = userSettingService.GetUserSetting<bool>(ASUserSettingConstants.ShowCLI);

            // Set the preview count
            this.PreviewPicturesToScan.Add(10);
            this.PreviewPicturesToScan.Add(15);
            this.PreviewPicturesToScan.Add(20);
            this.PreviewPicturesToScan.Add(25);
            this.PreviewPicturesToScan.Add(30);
            this.selectedPreviewCount = this.userSettingService.GetUserSetting<int>(UserSettingConstants.PreviewScanCount);

            // x264 step
            this.ConstantQualityGranularity.Add("1.0");
            this.ConstantQualityGranularity.Add("0.50");
            this.ConstantQualityGranularity.Add("0.25");
            this.ConstantQualityGranularity.Add("0.20");
            this.SelectedGranulairty = userSettingService.GetUserSetting<double>(ASUserSettingConstants.X264Step).ToString(new CultureInfo("en-US"));

            // Min Title Length
            this.minLength = this.userSettingService.GetUserSetting<int>(ASUserSettingConstants.MinScanDuration);

            // Use Experimental dvdnav
            if (userSettingService.GetUserSetting<bool>(ASUserSettingConstants.DisableLibDvdNav))
            {
                this.disableLibdvdNav = true;
            }

            this.isLoading = false;
        }

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