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
    using System.ComponentModel;
    using System.Diagnostics;
    using System.Globalization;
    using System.IO;
    using System.Linq;
    using System.Windows;

    using Caliburn.Micro;

    using HandBrake.ApplicationServices.Model;
    using HandBrake.ApplicationServices.Utilities;

    using HandBrakeWPF.Model;
    using HandBrakeWPF.Services.Interfaces;
    using HandBrakeWPF.ViewModels.Interfaces;

    using Ookii.Dialogs.Wpf;

    /// <summary>
    /// The Options View Model
    /// </summary>
    public class OptionsViewModel : ViewModelBase, IOptionsViewModel
    {
        #region Constants and Fields

        /// <summary>
        /// Backing field for the user setting service.
        /// </summary>
        private readonly IUserSettingService userSettingService;

        /// <summary>
        /// The Shell View Model
        /// </summary>
        private readonly IShellViewModel shellViewModel;

        /// <summary>
        /// Backing field for the update service.
        /// </summary>
        private readonly IUpdateService updateService;

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
        /// The remove underscores.
        /// </summary>
        private bool removeUnderscores;

        /// <summary>
        /// The selected granulairty.
        /// </summary>
        private string selectedGranulairty;

        /// <summary>
        /// The selected mp 4 extension.
        /// </summary>
        private int selectedMp4Extension;

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
        /// Backing field for clear queue on encode completed.
        /// </summary>
        private bool clearQueueOnEncodeCompleted;

        /// <summary>
        /// The options tab that is selected.
        /// </summary>
        private OptionsTab selectedTab;

        /// <summary>
        /// Update Message
        /// </summary>
        private string updateMessage;

        /// <summary>
        /// Update Available
        /// </summary>
        private bool updateAvailable;

        /// <summary>
        /// Download progress backing field.
        /// </summary>
        private int downloadProgressPercentage;

        /// <summary>
        /// Backing field for update info.
        /// </summary>
        private UpdateCheckInformation updateInfo;

        /// <summary>
        /// The enable process isolation.
        /// </summary>
        private bool enableProcessIsolation;

        /// <summary>
        /// The server port.
        /// </summary>
        private int serverPort;

        /// <summary>
        /// Backing field for EnableLibHb
        /// </summary>
        private bool enableLibHb;

        /// <summary>
        /// The show advanced tab backing field.
        /// </summary>
        private bool showAdvancedTab;

        /// <summary>
        /// The enable static preview.
        /// </summary>
        private bool enableStaticPreview;

        /// <summary>
        /// The remove punctuation.
        /// </summary>
        private bool removePunctuation;

        /// <summary>
        /// The use system colours for styles.
        /// </summary>
        private bool useSystemColoursForStyles;

        /// <summary>
        /// The reset when done action.
        /// </summary>
        private bool resetWhenDoneAction;

        /// <summary>
        /// The selected scaling mode.
        /// </summary>
        private VideoScaler selectedScalingMode;

        /// <summary>
        /// The enable dxva decoding.
        /// </summary>
        private bool enableDxvaDecoding;

        /// <summary>
        /// The disable quick sync decoding.
        /// </summary>
        private bool disableQuickSyncDecoding;

        /// <summary>
        /// The is cl scaling.
        /// </summary>
        private bool isClScaling;

        #endregion

        #region Constructors and Destructors

        /// <summary>
        /// Initializes a new instance of the <see cref="OptionsViewModel"/> class.
        /// </summary>
        /// <param name="userSettingService">
        /// The user Setting Service.
        /// </param>
        /// <param name="shellViewModel">
        /// The shell View Model.
        /// </param>
        /// <param name="updateService">
        /// The update Service.
        /// </param>
        public OptionsViewModel(IUserSettingService userSettingService, IShellViewModel shellViewModel, IUpdateService updateService)
        {
            this.Title = "Options";
            this.userSettingService = userSettingService;
            this.shellViewModel = shellViewModel;
            this.updateService = updateService;
            this.OnLoad();

            this.SelectedTab = OptionsTab.General;
            this.UpdateMessage = "Click 'Check for Updates' to check for new versions";
        }

        #endregion

        #region Window Properties

        /// <summary>
        /// Gets or sets SelectedTab.
        /// </summary>
        public OptionsTab SelectedTab
        {
            get
            {
                return this.selectedTab;
            }

            set
            {
                this.selectedTab = value;
                this.NotifyOfPropertyChange(() => this.SelectedTab);
            }
        }

        /// <summary>
        /// Gets or sets the about view model.
        /// </summary>
        public IAboutViewModel AboutViewModel { get; set; }

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
        /// Gets or sets a value indicating whether reset when done action.
        /// </summary>
        public bool ResetWhenDoneAction
        {
            get
            {
                return this.resetWhenDoneAction;
            }

            set
            {
                this.resetWhenDoneAction = value;
                this.NotifyOfPropertyChange("ResetWhenDoneAction");
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

        /// <summary>
        /// Gets or sets a value indicating whether use system colours.
        /// </summary>
        public bool UseSystemColoursForStylesForStyles
        {
            get
            {
                return this.useSystemColoursForStyles;
            }
            set
            {
                this.useSystemColoursForStyles = value;
                this.NotifyOfPropertyChange(() => UseSystemColoursForStylesForStyles);
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

        /// <summary>
        /// Gets or sets a value indicating whether remove punctuation.
        /// </summary>
        public bool RemovePunctuation
        {
            get
            {
                return this.removePunctuation;
            }
            set
            {
                this.removePunctuation = value;
                this.NotifyOfPropertyChange(() => RemovePunctuation);
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
        /// Gets or sets a value indicating whether ClearQueueOnEncodeCompleted.
        /// </summary>
        public bool ClearQueueOnEncodeCompleted
        {
            get
            {
                return this.clearQueueOnEncodeCompleted;
            }
            set
            {
                this.clearQueueOnEncodeCompleted = value;
                this.NotifyOfPropertyChange(() => this.ClearQueueOnEncodeCompleted);
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether ClearQueueOnEncodeCompleted.
        /// </summary>
        public bool EnableProcessIsolation
        {
            get
            {
                return this.enableProcessIsolation;
            }
            set
            {
                this.enableProcessIsolation = value;
                this.NotifyOfPropertyChange(() => this.EnableProcessIsolation);
            }
        }

        /// <summary>
        /// Gets or sets the server port.
        /// </summary>
        public int ServerPort
        {
            get
            {
                return this.serverPort;
            }
            set
            {
                this.serverPort = value;
                this.NotifyOfPropertyChange(() => this.ServerPort);
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether enable lib hb.
        /// </summary>
        public bool EnableLibHb
        {
            get
            {
                return this.enableLibHb;
            }
            set
            {
                this.enableLibHb = value;
                this.NotifyOfPropertyChange(() => this.EnableLibHb);
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether enable lib hb.
        /// </summary>
        public bool ShowAdvancedTab
        {
            get
            {
                return this.showAdvancedTab;
            }
            set
            {
                this.showAdvancedTab = value;
                this.NotifyOfPropertyChange(() => this.ShowAdvancedTab);
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether enable static preview.
        /// </summary>
        public bool EnableStaticPreview
        {
            get
            {
                return this.enableStaticPreview;
            }
            set
            {
                this.enableStaticPreview = value;
                this.NotifyOfPropertyChange(() => this.EnableStaticPreview);
            }
        }

        #endregion

        #region Video

        /// <summary>
        /// Gets or sets a value indicating whether disable quick sync decoding.
        /// </summary>
        public bool DisableQuickSyncDecoding
        {
            get
            {
                return this.disableQuickSyncDecoding;
            }
            set
            {
                if (value.Equals(this.disableQuickSyncDecoding))
                {
                    return;
                }
                this.disableQuickSyncDecoding = value;
                this.NotifyOfPropertyChange(() => this.DisableQuickSyncDecoding);
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether enable dxva decoding.
        /// </summary>
        public bool EnableDxvaDecoding
        {
            get
            {
                return this.enableDxvaDecoding;
            }
            set
            {
                if (value.Equals(this.enableDxvaDecoding))
                {
                    return;
                }
                this.enableDxvaDecoding = value;
                this.NotifyOfPropertyChange(() => this.EnableDxvaDecoding);
            }
        }

        /// <summary>
        /// Gets or sets the selected scaling mode.
        /// </summary>
        public VideoScaler SelectedScalingMode
        {
            get
            {
                return this.selectedScalingMode;
            }
            set
            {
                this.selectedScalingMode = value;
                this.IsClScaling = value == VideoScaler.BicubicCl;
            }
        }

        /// <summary>
        /// Gets a value indicating whether is quick sync available.
        /// </summary>
        public bool IsQuickSyncAvailable
        {
            get
            {
                return SystemInfo.IsQsvAvailable;
            }
        }

        /// <summary>
        /// Gets the scaling options.
        /// </summary>
        public BindingList<VideoScaler> ScalingOptions
        {
            get
            {
                return new BindingList<VideoScaler>(EnumHelper<VideoScaler>.GetEnumList().ToList());
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether is cl scaling.
        /// </summary>
        public bool IsClScaling
        {
            get
            {
                return this.isClScaling;
            }
            set
            {
                if (value.Equals(this.isClScaling))
                {
                    return;
                }
                this.isClScaling = value;
                this.NotifyOfPropertyChange(() => this.IsClScaling);
            }
        }

        #endregion

        #endregion

        #region About HandBrake

        /// <summary>
        /// Gets Version.
        /// </summary>
        public string Version
        {
            get
            {
                return string.Format("{0} - {1}", VersionHelper.GetVersion(), VersionHelper.GetPlatformBitnessVersion());
            }
        }

        /// <summary>
        /// Gets or sets UpdateMessage.
        /// </summary>
        public string UpdateMessage
        {
            get
            {
                return this.updateMessage;
            }
            set
            {
                this.updateMessage = value;
                this.NotifyOfPropertyChange(() => this.UpdateMessage);
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether UpdateAvailable.
        /// </summary>
        public bool UpdateAvailable
        {
            get
            {
                return this.updateAvailable;
            }
            set
            {
                this.updateAvailable = value;
                this.NotifyOfPropertyChange(() => this.UpdateAvailable);
            }
        }

        /// <summary>
        /// Gets or sets DownloadProgressPercentage.
        /// </summary>
        public int DownloadProgressPercentage
        {
            get
            {
                return this.downloadProgressPercentage;
            }
            set
            {
                this.downloadProgressPercentage = value;
                this.NotifyOfPropertyChange(() => this.DownloadProgressPercentage);
            }
        }

        #endregion

        #region Public Methods

        /// <summary>
        /// Load / Update the user settings.
        /// </summary>
        protected override void OnActivate()
        {
            this.OnLoad();
            base.OnActivate();
        }

        /// <summary>
        /// Close this window.
        /// </summary>
        public void Close()
        {
            this.Save();
            this.shellViewModel.DisplayWindow(ShellWindow.MainWindow);
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

        /// <summary>
        /// Download an Update
        /// </summary>
        public void DownloadUpdate()
        {
            this.UpdateMessage = "Preparing for Update ...";
            this.updateService.DownloadFile(this.updateInfo.DownloadFile, this.DownloadComplete, this.DownloadProgress);
        }

        /// <summary>
        /// Check for updates
        /// </summary>
        public void PerformUpdateCheck()
        {
            this.UpdateMessage = "Checking for Updates ...";
            this.updateService.CheckForUpdates(this.UpdateCheckComplete);
        }

        #endregion

        /// <summary>
        /// Load User Settings
        /// </summary>
        public override void OnLoad()
        {
            // #############################
            // General
            // #############################

            this.CheckForUpdates = this.userSettingService.GetUserSetting<bool>(UserSettingConstants.UpdateStatus);

            // Days between update checks
            this.checkForUpdatesFrequencies.Clear();
            this.checkForUpdatesFrequencies.Add("Daily");
            this.checkForUpdatesFrequencies.Add("Weekly");
            this.checkForUpdatesFrequencies.Add("Monthly");

            // TODO Refactor this.
            switch (this.userSettingService.GetUserSetting<int>(UserSettingConstants.DaysBetweenUpdateCheck))
            {
                case 1:
                    this.CheckForUpdatesFrequency = 0;
                    break;
                case 7:
                    this.CheckForUpdatesFrequency = 1;
                    break;
                default:
                    this.CheckForUpdatesFrequency = 2;
                    break;
            }

            // On Encode Completeion Action
            this.whenDoneOptions.Clear();
            this.whenDoneOptions.Add("Do nothing");
            this.whenDoneOptions.Add("Shutdown");
            this.whenDoneOptions.Add("Suspend");
            this.whenDoneOptions.Add("Hibernate");
            this.whenDoneOptions.Add("Lock System");
            this.whenDoneOptions.Add("Log off");
            this.whenDoneOptions.Add("Quit HandBrake");
            this.WhenDone = userSettingService.GetUserSetting<string>("WhenCompleteAction");
            if (this.userSettingService.GetUserSetting<bool>(UserSettingConstants.ResetWhenDoneAction))
            {
                this.WhenDone = "Do nothing";
                this.userSettingService.SetUserSetting(UserSettingConstants.WhenCompleteAction, "Do nothing");
            }

            this.GrowlAfterEncode = userSettingService.GetUserSetting<bool>(UserSettingConstants.GrowlEncode);
            this.GrowlAfterQueue = userSettingService.GetUserSetting<bool>(UserSettingConstants.GrowlQueue);
            this.SendFileAfterEncode = this.userSettingService.GetUserSetting<bool>(UserSettingConstants.SendFile);
            this.SendFileTo = Path.GetFileNameWithoutExtension(this.userSettingService.GetUserSetting<string>(UserSettingConstants.SendFileTo)) ?? string.Empty;
            this.SendFileToPath = this.userSettingService.GetUserSetting<string>(UserSettingConstants.SendFileTo) ?? string.Empty;
            this.Arguments = this.userSettingService.GetUserSetting<string>(UserSettingConstants.SendFileToArgs) ?? string.Empty;
            this.UseSystemColoursForStylesForStyles = this.userSettingService.GetUserSetting<bool>(UserSettingConstants.UseSystemColours);
            this.ResetWhenDoneAction = this.userSettingService.GetUserSetting<bool>(UserSettingConstants.ResetWhenDoneAction);

            // #############################
            // Output Settings
            // #############################

            // Enable auto naming feature.)
            this.AutomaticallyNameFiles = this.userSettingService.GetUserSetting<bool>(UserSettingConstants.AutoNaming);

            // Store the auto name path
            this.AutoNameDefaultPath = this.userSettingService.GetUserSetting<string>(UserSettingConstants.AutoNamePath) ?? string.Empty;
            if (string.IsNullOrEmpty(this.autoNameDefaultPath))
                this.AutoNameDefaultPath = "Click 'Browse' to set the default location";

            // Store auto name format
            this.AutonameFormat = this.userSettingService.GetUserSetting<string>(UserSettingConstants.AutoNameFormat) ?? string.Empty;

            // Use iPod/iTunes friendly .m4v extension for MP4 files.
            this.mp4ExtensionOptions.Clear();
            this.mp4ExtensionOptions.Add("Automatic");
            this.mp4ExtensionOptions.Add("Always use MP4");
            this.mp4ExtensionOptions.Add("Always use M4V");
            this.SelectedMp4Extension = this.userSettingService.GetUserSetting<int>(UserSettingConstants.UseM4v);

            // Remove Underscores
            this.RemoveUnderscores = this.userSettingService.GetUserSetting<bool>(UserSettingConstants.AutoNameRemoveUnderscore);

            // Title case
            this.ChangeToTitleCase = this.userSettingService.GetUserSetting<bool>(UserSettingConstants.AutoNameTitleCase);
            this.RemovePunctuation = this.userSettingService.GetUserSetting<bool>(UserSettingConstants.RemovePunctuation);

            // #############################
            // Picture Tab
            // #############################

            // VLC Path
            this.VLCPath = this.userSettingService.GetUserSetting<string>(UserSettingConstants.VLCPath) ?? string.Empty;

            // #############################
            // Video
            // #############################
            this.DisableQuickSyncDecoding = this.userSettingService.GetUserSetting<bool>(UserSettingConstants.DisableQuickSyncDecoding);
            this.EnableDxvaDecoding = this.userSettingService.GetUserSetting<bool>(UserSettingConstants.EnableDxva);
            this.SelectedScalingMode = this.userSettingService.GetUserSetting<VideoScaler>(UserSettingConstants.ScalingMode);

            // #############################
            // CLI
            // #############################

            // Priority level for encodes
            this.priorityLevelOptions.Clear();
            this.priorityLevelOptions.Add("High");
            this.priorityLevelOptions.Add("Above Normal");
            this.priorityLevelOptions.Add("Normal");
            this.priorityLevelOptions.Add("Below Normal");
            this.priorityLevelOptions.Add("Low");
            this.SelectedPriority = userSettingService.GetUserSetting<string>(UserSettingConstants.ProcessPriority);

            this.PreventSleep = userSettingService.GetUserSetting<bool>(UserSettingConstants.PreventSleep);

            // Log Verbosity Level
            this.logVerbosityOptions.Clear();
            this.logVerbosityOptions.Add(0);
            this.logVerbosityOptions.Add(1);
            this.logVerbosityOptions.Add(2);
            this.SelectedVerbosity = userSettingService.GetUserSetting<int>(UserSettingConstants.Verbosity);

            // Logs
            this.CopyLogToEncodeDirectory = userSettingService.GetUserSetting<bool>(UserSettingConstants.SaveLogWithVideo);
            this.CopyLogToSepcficedLocation = userSettingService.GetUserSetting<bool>(UserSettingConstants.SaveLogToCopyDirectory);

            // The saved log path
            this.LogDirectory = userSettingService.GetUserSetting<string>(UserSettingConstants.SaveLogCopyDirectory) ?? string.Empty;

            this.ClearOldOlgs = this.userSettingService.GetUserSetting<bool>(UserSettingConstants.ClearOldLogs);

            // #############################
            // Advanced
            // #############################

            // Minimise to Tray
            this.MinimiseToTray = this.userSettingService.GetUserSetting<bool>(UserSettingConstants.MainWindowMinimize);
            this.DisablePresetUpdateCheckNotification = this.userSettingService.GetUserSetting<bool>(UserSettingConstants.PresetNotification);
            this.ClearQueueOnEncodeCompleted = userSettingService.GetUserSetting<bool>(UserSettingConstants.ClearCompletedFromQueue);
            this.ShowAdvancedTab = userSettingService.GetUserSetting<bool>(UserSettingConstants.ShowAdvancedTab);

            // Set the preview count
            this.PreviewPicturesToScan.Clear();
            this.PreviewPicturesToScan.Add(10);
            this.PreviewPicturesToScan.Add(15);
            this.PreviewPicturesToScan.Add(20);
            this.PreviewPicturesToScan.Add(25);
            this.PreviewPicturesToScan.Add(30);
            this.PreviewPicturesToScan.Add(35);
            this.PreviewPicturesToScan.Add(40);
            this.PreviewPicturesToScan.Add(45);
            this.PreviewPicturesToScan.Add(50);
            this.PreviewPicturesToScan.Add(55);
            this.PreviewPicturesToScan.Add(60);
            this.SelectedPreviewCount = this.userSettingService.GetUserSetting<int>(UserSettingConstants.PreviewScanCount);

            // x264 step
            this.ConstantQualityGranularity.Clear();
            this.ConstantQualityGranularity.Add("1.00");
            this.ConstantQualityGranularity.Add("0.50");
            this.ConstantQualityGranularity.Add("0.25");
            this.SelectedGranulairty = userSettingService.GetUserSetting<double>(UserSettingConstants.X264Step).ToString("0.00", CultureInfo.InvariantCulture);

            // Min Title Length
            this.MinLength = this.userSettingService.GetUserSetting<int>(UserSettingConstants.MinScanDuration);

            // Use dvdnav
            this.DisableLibdvdNav = userSettingService.GetUserSetting<bool>(UserSettingConstants.DisableLibDvdNav);

            this.EnableStaticPreview = userSettingService.GetUserSetting<bool>(UserSettingConstants.EnableStaticPreview);            

            int port;
            int.TryParse(userSettingService.GetUserSetting<string>(UserSettingConstants.ServerPort), out port);
            this.ServerPort = port;
            this.EnableProcessIsolation = userSettingService.GetUserSetting<bool>(UserSettingConstants.EnableProcessIsolation);
            this.EnableLibHb = userSettingService.GetUserSetting<bool>(UserSettingConstants.EnableLibHb);
        }

        /// <summary>
        /// Save the settings selected
        /// </summary>
        private void Save()
        {
            /* General */
            this.userSettingService.SetUserSetting(UserSettingConstants.UpdateStatus, this.CheckForUpdates);
            this.userSettingService.SetUserSetting(UserSettingConstants.DaysBetweenUpdateCheck, this.CheckForUpdatesFrequency);
            this.userSettingService.SetUserSetting(UserSettingConstants.WhenCompleteAction, this.WhenDone);
            this.userSettingService.SetUserSetting(UserSettingConstants.GrowlQueue, this.GrowlAfterQueue);
            this.userSettingService.SetUserSetting(UserSettingConstants.GrowlEncode, this.GrowlAfterEncode);
            this.userSettingService.SetUserSetting(UserSettingConstants.SendFileTo, this.SendFileToPath);
            this.userSettingService.SetUserSetting(UserSettingConstants.SendFile, this.SendFileAfterEncode);
            this.userSettingService.SetUserSetting(UserSettingConstants.SendFileToArgs, this.Arguments);
            this.userSettingService.SetUserSetting(UserSettingConstants.UseSystemColours, this.UseSystemColoursForStylesForStyles);
            this.userSettingService.SetUserSetting(UserSettingConstants.ResetWhenDoneAction, this.ResetWhenDoneAction);

            /* Output Files */
            this.userSettingService.SetUserSetting(UserSettingConstants.AutoNaming, this.AutomaticallyNameFiles);
            this.userSettingService.SetUserSetting(UserSettingConstants.AutoNameFormat, this.AutonameFormat);
            this.userSettingService.SetUserSetting(UserSettingConstants.AutoNamePath, this.AutoNameDefaultPath);
            this.userSettingService.SetUserSetting(UserSettingConstants.UseM4v, this.SelectedMp4Extension);
            this.userSettingService.SetUserSetting(UserSettingConstants.AutoNameRemoveUnderscore, this.RemoveUnderscores);
            this.userSettingService.SetUserSetting(UserSettingConstants.AutoNameTitleCase, this.ChangeToTitleCase);
            this.userSettingService.SetUserSetting(UserSettingConstants.RemovePunctuation, this.RemovePunctuation);

            /* Previews */
            this.userSettingService.SetUserSetting(UserSettingConstants.VLCPath, this.VLCPath);
            
            /* Video */
            this.userSettingService.SetUserSetting(UserSettingConstants.DisableQuickSyncDecoding, this.DisableQuickSyncDecoding);
            this.userSettingService.SetUserSetting(UserSettingConstants.EnableDxva, this.EnableDxvaDecoding);
            this.userSettingService.SetUserSetting(UserSettingConstants.ScalingMode, this.SelectedScalingMode);

            /* System and Logging */
            userSettingService.SetUserSetting(UserSettingConstants.ProcessPriority, this.SelectedPriority);
            userSettingService.SetUserSetting(UserSettingConstants.PreventSleep, this.PreventSleep);
            userSettingService.SetUserSetting(UserSettingConstants.Verbosity, this.SelectedVerbosity);
            userSettingService.SetUserSetting(UserSettingConstants.SaveLogWithVideo, this.CopyLogToEncodeDirectory);
            userSettingService.SetUserSetting(UserSettingConstants.SaveLogToCopyDirectory, this.CopyLogToSepcficedLocation);
            userSettingService.SetUserSetting(UserSettingConstants.SaveLogCopyDirectory, this.LogDirectory);
            userSettingService.SetUserSetting(UserSettingConstants.ClearOldLogs, this.ClearOldOlgs);

            /* Advanced */
            userSettingService.SetUserSetting(UserSettingConstants.MainWindowMinimize, this.MinimiseToTray);
            userSettingService.SetUserSetting(UserSettingConstants.PresetNotification, this.DisablePresetUpdateCheckNotification);
            userSettingService.SetUserSetting(UserSettingConstants.ClearCompletedFromQueue, this.ClearQueueOnEncodeCompleted);
            userSettingService.SetUserSetting(UserSettingConstants.PreviewScanCount, this.SelectedPreviewCount);
            userSettingService.SetUserSetting(UserSettingConstants.X264Step, double.Parse(this.SelectedGranulairty, CultureInfo.InvariantCulture));
            userSettingService.SetUserSetting(UserSettingConstants.ShowAdvancedTab, this.ShowAdvancedTab);
            userSettingService.SetUserSetting(UserSettingConstants.EnableStaticPreview, this.EnableStaticPreview);

            int value;
            if (int.TryParse(this.MinLength.ToString(CultureInfo.InvariantCulture), out value))
            {
                this.userSettingService.SetUserSetting(UserSettingConstants.MinScanDuration, value);
            }

            userSettingService.SetUserSetting(UserSettingConstants.DisableLibDvdNav, this.DisableLibdvdNav);
            userSettingService.SetUserSetting(UserSettingConstants.EnableProcessIsolation, this.EnableProcessIsolation);
            userSettingService.SetUserSetting(UserSettingConstants.ServerPort, this.ServerPort.ToString(CultureInfo.InvariantCulture));
            userSettingService.SetUserSetting(UserSettingConstants.EnableLibHb, this.EnableLibHb);
        }

        /// <summary>
        /// Update Check Complete
        /// </summary>
        /// <param name="info">
        /// The info.
        /// </param>
        private void UpdateCheckComplete(UpdateCheckInformation info)
        {
            this.updateInfo = info;
            if (info.NewVersionAvailable)
            {
                this.UpdateMessage = "A New Update is Available!";
                this.UpdateAvailable = true;
            }
            else
            {
                this.UpdateMessage = "There are no new updates at this time.";
                this.UpdateAvailable = false;
            }
        }

        /// <summary>
        /// Download Progress Action
        /// </summary>
        /// <param name="info">
        /// The info.
        /// </param>
        private void DownloadProgress(DownloadStatus info)
        {
            if (info.TotalBytes == 0 || info.BytesRead == 0)
            {
                this.UpdateAvailable = false;
                this.UpdateMessage = info.WasSuccessful ? "Update Downloaded" : "Update Service Unavailable. You can try downloading the update from https://handbrake.fr";
                return;
            }

            long p = (info.BytesRead * 100) / info.TotalBytes;
            int progress;
            int.TryParse(p.ToString(CultureInfo.InvariantCulture), out progress);
            this.DownloadProgressPercentage = progress;
            this.UpdateMessage = string.Format(
                "Downloading... {0}% - {1}k of {2}k", this.DownloadProgressPercentage, (info.BytesRead / 1024), (info.TotalBytes / 1024));
        }

        /// <summary>
        /// Download Complete Action
        /// </summary>
        /// <param name="info">
        /// The info.
        /// </param>
        private void DownloadComplete(DownloadStatus info)
        {
            this.UpdateAvailable = false;
            this.UpdateMessage = info.WasSuccessful ? "Update Downloaded" : "Update Failed. You can try downloading the update from https://handbrake.fr";

            Process.Start(Path.Combine(Path.GetTempPath(), "handbrake-setup.exe"));
            Execute.OnUIThread(() => Application.Current.Shutdown());
        }

        /// <summary>
        /// The goto tab.
        /// </summary>
        /// <param name="tab">
        /// The tab.
        /// </param>
        public void GotoTab(OptionsTab tab)
        {
            this.SelectedTab = tab;
        }
    }
}