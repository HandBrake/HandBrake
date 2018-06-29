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

    using HandBrake.Interop.Model;
    using HandBrake.Interop.Utilities;

    using HandBrakeWPF.Model;
    using HandBrakeWPF.Properties;
    using HandBrakeWPF.Services.Interfaces;
    using HandBrakeWPF.Utilities;
    using HandBrakeWPF.ViewModels.Interfaces;

    using Microsoft.Win32;

    using Ookii.Dialogs.Wpf;

    using Execute = Caliburn.Micro.Execute;
    using SystemInfo = HandBrake.Interop.Utilities.SystemInfo;

    /// <summary>
    /// The Options View Model
    /// </summary>
    public class OptionsViewModel : ViewModelBase, IOptionsViewModel
    {
        #region Constants and Fields

        private readonly IUserSettingService userSettingService;
        private readonly IUpdateService updateService;
        private readonly IErrorService errorService;

        private string arguments;
        private string autoNameDefaultPath;
        private bool automaticallyNameFiles;
        private string autonameFormat;
        private bool changeToTitleCase;
        private bool checkForUpdates;
        private BindingList<string> checkForUpdatesFrequencies = new BindingList<string>();
        private int checkForUpdatesFrequency;
        private bool clearOldOlgs;
        private BindingList<string> constantQualityGranularity = new BindingList<string>();
        private bool copyLogToEncodeDirectory;
        private bool copyLogToSepcficedLocation;
        private bool disableLibdvdNav;
        private string logDirectory;
        private BindingList<int> logVerbosityOptions = new BindingList<int>();
        private long minLength;
        private bool minimiseToTray;
        private BindingList<string> mp4ExtensionOptions = new BindingList<string>();
        private bool preventSleep;
        private BindingList<int> previewPicturesToScan = new BindingList<int>();
        private BindingList<string> priorityLevelOptions = new BindingList<string>();
        private bool removeUnderscores;
        private string selectedGranulairty;
        private int selectedMp4Extension;
        private int selectedPreviewCount;
        private string selectedPriority;
        private int selectedVerbosity;
        private bool sendFileAfterEncode;
        private string sendFileTo;
        private string sendFileToPath;
        private string vlcPath;
        private string whenDone;
        private BindingList<string> whenDoneOptions = new BindingList<string>();
        private bool clearQueueOnEncodeCompleted;
        private OptionsTab selectedTab;
        private string updateMessage;
        private bool updateAvailable;
        private int downloadProgressPercentage;
        private UpdateCheckInformation updateInfo;
        private bool showAdvancedTab;
        private bool removePunctuation;
        private bool resetWhenDoneAction;

        private bool enableQuickSyncDecoding;
        private bool showQueueInline;
        private bool pauseOnLowDiskspace;
        private long pauseOnLowDiskspaceLevel;
        private bool useQsvDecodeForNonQsvEnc;
        private bool showStatusInTitleBar;
        private bool showPreviewOnSummaryTab;

        private string whenDoneAudioFile;
        private bool playSoundWhenDone;
        private bool playSoundWhenQueueDone;

        private bool enableQuickSyncEncoding;

        private bool enableVceEncoder;

        private bool enableNvencEncoder;

        #endregion

        #region Constructors and Destructors

        /// <summary>
        /// Initializes a new instance of the <see cref="OptionsViewModel"/> class.
        /// </summary>
        /// <param name="userSettingService">
        /// The user Setting Service.
        /// </param>
        /// <param name="updateService">
        /// The update Service.
        /// </param>
        /// <param name="aboutViewModel">
        /// The about View Model.
        /// </param>
        /// <param name="errorService">
        /// The error Service.
        /// </param>
        public OptionsViewModel(IUserSettingService userSettingService, IUpdateService updateService, IAboutViewModel aboutViewModel, IErrorService errorService)
        {
            this.Title = "Options";
            this.userSettingService = userSettingService;
            this.updateService = updateService;
            this.errorService = errorService;
            this.AboutViewModel = aboutViewModel;
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

        public bool IsUWP
        {
            get
            {
                return UwpDetect.IsUWP();
            }
        }

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
        /// Gets or sets a value indicating whether show queue inline.
        /// </summary>
        public bool ShowQueueInline
        {
            get
            {
                return this.showQueueInline;
            }
            set
            {
                if (value == this.showQueueInline)
                {
                    return;
                }
                this.showQueueInline = value;
                this.NotifyOfPropertyChange(() => this.ShowQueueInline);
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether to show encode status in the tile bar.
        /// </summary>
        public bool ShowStatusInTitleBar
        {
            get
            {
                return this.showStatusInTitleBar;
            }
            set
            {
                if (value == this.showStatusInTitleBar) return;
                this.showStatusInTitleBar = value;
                this.NotifyOfPropertyChange(() => this.ShowStatusInTitleBar);
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether to show previews in the summary tab.
        /// </summary>
        public bool ShowPreviewOnSummaryTab
        {
            get
            {
                return this.showPreviewOnSummaryTab;
            }
            set
            {
                if (value == this.showPreviewOnSummaryTab) return;
                this.showPreviewOnSummaryTab = value;
                this.NotifyOfPropertyChange(() => this.ShowPreviewOnSummaryTab);
            }
        }

        /// <summary>
        /// When Done Audio File
        /// </summary>
        public string WhenDoneAudioFile
        {
            get
            {
                return this.whenDoneAudioFile;
            }
            set
            {
                if (value == this.whenDoneAudioFile) return;
                this.whenDoneAudioFile = value;
                this.NotifyOfPropertyChange(() => this.WhenDoneAudioFile);
            }
        }

        /// <summary>
        /// When Done Audio File - File Path
        /// </summary>
        public string WhenDoneAudioFileFullPath { get; set; }

        /// <summary>
        /// Play a sound when an encode or queue finishes.
        /// </summary>
        public bool PlaySoundWhenDone
        {
            get
            {
                return this.playSoundWhenDone;
            }
            set
            {
                if (value == this.playSoundWhenDone) return;
                this.playSoundWhenDone = value;
                this.NotifyOfPropertyChange(() => this.PlaySoundWhenDone);
            }
        }

        public bool PlaySoundWhenQueueDone
        {
            get
            {
                return this.playSoundWhenQueueDone;
            }
            set
            {
                if (value == this.playSoundWhenQueueDone) return;
                this.playSoundWhenQueueDone = value;
                this.NotifyOfPropertyChange(() => this.PlaySoundWhenQueueDone);
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
                if (this.IsValidAutonameFormat(value, false))
                {
                    this.autonameFormat = value;
                }

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
        /// Gets or sets a value indicating whether HandBrake should pause on low disk space.
        /// </summary>
        public bool PauseOnLowDiskspace
        {
            get
            {
                return this.pauseOnLowDiskspace;
            }

            set
            {
                this.pauseOnLowDiskspace = value;
                this.NotifyOfPropertyChange(() => this.PauseOnLowDiskspace);
            }
        }

        /// <summary>
        /// Get or sets the value that HB warns about low disk space.
        /// </summary>
        public long PauseOnLowDiskspaceLevel
        {
            get
            {
                return this.pauseOnLowDiskspaceLevel;
            }

            set
            {
                this.pauseOnLowDiskspaceLevel = value;
                this.NotifyOfPropertyChange(() => this.pauseOnLowDiskspaceLevel);
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
                this.NotifyOfPropertyChange();

                // Set the Process Priority
                switch (value)
                {
                    case "Realtime":
                        Process.GetCurrentProcess().PriorityClass = ProcessPriorityClass.RealTime;
                        break;
                    case "High":
                        Process.GetCurrentProcess().PriorityClass = ProcessPriorityClass.High;
                        break;
                    case "Above Normal":
                        Process.GetCurrentProcess().PriorityClass = ProcessPriorityClass.AboveNormal;
                        break;
                    case "Normal":
                        Process.GetCurrentProcess().PriorityClass = ProcessPriorityClass.Normal;
                        break;
                    case "Low":
                        Process.GetCurrentProcess().PriorityClass = ProcessPriorityClass.Idle;
                        break;
                    default:
                        Process.GetCurrentProcess().PriorityClass = ProcessPriorityClass.BelowNormal;
                        break;
                }
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

        #endregion

        #region Video

        public bool EnableQuickSyncEncoding
        {
            get => this.enableQuickSyncEncoding && this.IsQuickSyncAvailable;
            set
            {
                if (value == this.enableQuickSyncEncoding)
                {
                    return;
                }

                this.enableQuickSyncEncoding = value;
                this.NotifyOfPropertyChange(() => this.EnableQuickSyncEncoding);
            }
        }

        public bool EnableVceEncoder
        {
            get => this.enableVceEncoder && this.IsVceAvailable;
            set
            {
                if (value == this.enableVceEncoder)
                {
                    return;
                }

                this.enableVceEncoder = value;
                this.NotifyOfPropertyChange(() => this.EnableVceEncoder);
            }
        }

        public bool EnableNvencEncoder
        {
            get => this.enableNvencEncoder && this.IsNvencAvailable;
            set
            {
                if (value == this.enableNvencEncoder)
                {
                    return;
                }

                this.enableNvencEncoder = value;
                this.NotifyOfPropertyChange(() => this.EnableNvencEncoder);
            }
        }

        public bool EnableQuickSyncDecoding
        {
            get
            {
                return this.enableQuickSyncDecoding;
            }

            set
            {
                if (value.Equals(this.enableQuickSyncDecoding))
                {
                    return;
                }
                this.enableQuickSyncDecoding = value;
                this.NotifyOfPropertyChange(() => this.EnableQuickSyncDecoding);
                this.NotifyOfPropertyChange(() => this.IsUseQsvDecAvailable);
            }
        }

        public VideoScaler SelectedScalingMode { get; set; }

        public bool IsQuickSyncAvailable
        {
            get
            {
                return SystemInfo.IsQsvAvailable;
            }
        }

        public bool IsVceAvailable
        {
            get
            {
                string foundGpu = Utilities.SystemInfo.GetGPUInfo.FirstOrDefault(g => g.Contains("AMD"));
                return SystemInfo.IsVceH264Available && !string.IsNullOrEmpty(foundGpu);
            }
        }

        public bool IsNvencAvailable
        {
            get
            {
                return SystemInfo.IsNVEncH264Available;
            }
        }

        /// <summary>
        /// Gets a value indicating whether is use qsv dec available.
        /// </summary>
        public bool IsUseQsvDecAvailable
        {
            get
            {
                return IsQuickSyncAvailable && this.EnableQuickSyncDecoding;
            }
        }

        public bool UseQSVDecodeForNonQSVEnc
        {
            get
            {
                return this.useQsvDecodeForNonQsvEnc;
            }

            set
            {
                if (value == this.useQsvDecodeForNonQsvEnc) return;
                this.useQsvDecodeForNonQsvEnc = value;
                this.NotifyOfPropertyChange(() => this.UseQSVDecodeForNonQSVEnc);
            }
        }

        public BindingList<VideoScaler> ScalingOptions
        {
            get
            {
                return new BindingList<VideoScaler>(EnumHelper<VideoScaler>.GetEnumList().ToList());
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
                return string.Format("{0}", VersionHelper.GetVersion());
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
        /// Close this window.
        /// </summary>
        public void Close()
        {
            this.Save();

            IShellViewModel shellViewModel = IoC.Get<IShellViewModel>();
            shellViewModel.DisplayWindow(ShellWindow.MainWindow);
        }

        /// <summary>
        /// Browse - Send File To
        /// </summary>
        public void BrowseSendFileTo()
        {
            OpenFileDialog dialog = new OpenFileDialog { Filter = "All files (*.*)|*.*", FileName = this.sendFileToPath };
            bool? dialogResult = dialog.ShowDialog();
            if (dialogResult.HasValue && dialogResult.Value)
            {
                this.SendFileTo = Path.GetFileNameWithoutExtension(dialog.FileName);
                this.sendFileToPath = dialog.FileName;
            }
        }

        /// <summary>
        /// Browse Auto Name Path
        /// </summary>
        public void BrowseAutoNamePath()
        {
            VistaFolderBrowserDialog dialog = new VistaFolderBrowserDialog { Description = "Please select a folder.", UseDescriptionForTitle = true, SelectedPath = this.AutoNameDefaultPath };
            bool? dialogResult = dialog.ShowDialog();
            if (dialogResult.HasValue && dialogResult.Value)
            {
                this.AutoNameDefaultPath = dialog.SelectedPath;
            }
        }

        /// <summary>
        /// Browse VLC Path
        /// </summary>
        public void BrowseVlcPath()
        {
            OpenFileDialog dialog = new OpenFileDialog { Filter = "All files (*.exe)|*.exe", FileName = this.VLCPath };
            bool? dialogResult = dialog.ShowDialog();
            if (dialogResult.HasValue && dialogResult.Value)
            {
                this.VLCPath = dialog.FileName;
            }
        }

        /// <summary>
        /// Browse - Log Path
        /// </summary>
        public void BrowseLogPath()
        {
            VistaFolderBrowserDialog dialog = new VistaFolderBrowserDialog { Description = "Please select a folder.", UseDescriptionForTitle = true, SelectedPath = this.LogDirectory };
            bool? dialogResult = dialog.ShowDialog();
            if (dialogResult.HasValue && dialogResult.Value)
            {
                this.LogDirectory = dialog.SelectedPath;
            }
        }

        /// <summary>
        /// View the Default Log Directory for HandBrake
        /// </summary>
        public void ViewLogDirectory()
        {
            string logDir = DirectoryUtilities.GetLogDirectory();
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
            this.updateService.DownloadFile(this.updateInfo.DownloadFile, this.updateInfo.Signature, this.DownloadComplete, this.DownloadProgress);
        }

        /// <summary>
        /// Check for updates
        /// </summary>
        public void PerformUpdateCheck()
        {
            this.UpdateMessage = "Checking for Updates ...";
            this.updateService.CheckForUpdates(this.UpdateCheckComplete);
        }

        /// <summary>
        /// Browse - Send File To
        /// </summary>
        public void BrowseWhenDoneAudioFile()
        {
            OpenFileDialog dialog = new OpenFileDialog() { Filter = "All Files|*.wav;*.mp3", FileName = this.WhenDoneAudioFileFullPath };
            bool? dialogResult = dialog.ShowDialog();
            if (dialogResult.HasValue && dialogResult.Value)
            {
                this.WhenDoneAudioFile = Path.GetFileNameWithoutExtension(dialog.FileName);
                this.WhenDoneAudioFileFullPath = dialog.FileName;
            }
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
            this.checkForUpdatesFrequencies.Add("Weekly");
            this.checkForUpdatesFrequencies.Add("Monthly");

            this.CheckForUpdatesFrequency = this.userSettingService.GetUserSetting<int>(UserSettingConstants.DaysBetweenUpdateCheck);
            if (this.CheckForUpdatesFrequency > 1)
            {
                this.CheckForUpdatesFrequency = 1;
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
            }

            this.SendFileAfterEncode = this.userSettingService.GetUserSetting<bool>(UserSettingConstants.SendFile);
            this.SendFileTo = Path.GetFileNameWithoutExtension(this.userSettingService.GetUserSetting<string>(UserSettingConstants.SendFileTo)) ?? string.Empty;
            this.SendFileToPath = this.userSettingService.GetUserSetting<string>(UserSettingConstants.SendFileTo) ?? string.Empty;
            this.Arguments = this.userSettingService.GetUserSetting<string>(UserSettingConstants.SendFileToArgs) ?? string.Empty;
            this.ResetWhenDoneAction = this.userSettingService.GetUserSetting<bool>(UserSettingConstants.ResetWhenDoneAction);
            this.ShowQueueInline = this.userSettingService.GetUserSetting<bool>(UserSettingConstants.ShowQueueInline);
            this.ShowStatusInTitleBar = this.userSettingService.GetUserSetting<bool>(UserSettingConstants.ShowStatusInTitleBar);
            this.ShowPreviewOnSummaryTab = this.userSettingService.GetUserSetting<bool>(UserSettingConstants.ShowPreviewOnSummaryTab);
            this.WhenDoneAudioFile = Path.GetFileNameWithoutExtension(this.userSettingService.GetUserSetting<string>(UserSettingConstants.WhenDoneAudioFile)) ?? string.Empty;
            this.WhenDoneAudioFileFullPath = this.userSettingService.GetUserSetting<string>(UserSettingConstants.WhenDoneAudioFile);
            this.PlaySoundWhenDone = this.userSettingService.GetUserSetting<bool>(UserSettingConstants.PlaySoundWhenDone);
            this.PlaySoundWhenQueueDone = this.userSettingService.GetUserSetting<bool>(UserSettingConstants.PlaySoundWhenQueueDone);

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
            string anf = this.userSettingService.GetUserSetting<string>(UserSettingConstants.AutoNameFormat) ?? string.Empty;
            this.AutonameFormat = this.IsValidAutonameFormat(anf, true) ? anf : "{source}-{title}";

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
            this.EnableQuickSyncDecoding = this.userSettingService.GetUserSetting<bool>(UserSettingConstants.EnableQuickSyncDecoding);
            this.SelectedScalingMode = this.userSettingService.GetUserSetting<VideoScaler>(UserSettingConstants.ScalingMode);
            this.UseQSVDecodeForNonQSVEnc = this.userSettingService.GetUserSetting<bool>(UserSettingConstants.UseQSVDecodeForNonQSVEnc);

            this.EnableQuickSyncEncoding = this.userSettingService.GetUserSetting<bool>(UserSettingConstants.EnableQuickSyncEncoding);
            this.EnableVceEncoder = this.userSettingService.GetUserSetting<bool>(UserSettingConstants.EnableVceEncoder);
            this.EnableNvencEncoder = this.userSettingService.GetUserSetting<bool>(UserSettingConstants.EnableNvencEncoder);

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
            this.PauseOnLowDiskspace = userSettingService.GetUserSetting<bool>(UserSettingConstants.PauseOnLowDiskspace);
            this.PauseOnLowDiskspaceLevel = this.userSettingService.GetUserSetting<long>(UserSettingConstants.PauseOnLowDiskspaceLevel);

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
        }

        /// <summary>
        /// Some settings can be changed outside of this window. This will refresh their UI controls.
        /// </summary>
        public void UpdateSettings()
        {
            this.WhenDone = userSettingService.GetUserSetting<string>("WhenCompleteAction");
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

        /// <summary>
        /// Load / Update the user settings.
        /// </summary>
        protected override void OnActivate()
        {
            this.OnLoad();
            base.OnActivate();
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
            this.userSettingService.SetUserSetting(UserSettingConstants.SendFileTo, this.SendFileToPath);
            this.userSettingService.SetUserSetting(UserSettingConstants.SendFile, this.SendFileAfterEncode);
            this.userSettingService.SetUserSetting(UserSettingConstants.SendFileToArgs, this.Arguments);
            this.userSettingService.SetUserSetting(UserSettingConstants.ResetWhenDoneAction, this.ResetWhenDoneAction);
            this.userSettingService.SetUserSetting(UserSettingConstants.ShowQueueInline, this.ShowQueueInline);
            this.userSettingService.SetUserSetting(UserSettingConstants.ShowStatusInTitleBar, this.ShowStatusInTitleBar);
            this.userSettingService.SetUserSetting(UserSettingConstants.ShowPreviewOnSummaryTab, this.ShowPreviewOnSummaryTab);
            this.userSettingService.SetUserSetting(UserSettingConstants.PlaySoundWhenDone, this.PlaySoundWhenDone);
            this.userSettingService.SetUserSetting(UserSettingConstants.PlaySoundWhenQueueDone, this.PlaySoundWhenQueueDone);
            this.userSettingService.SetUserSetting(UserSettingConstants.WhenDoneAudioFile, this.WhenDoneAudioFileFullPath);

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
            this.userSettingService.SetUserSetting(UserSettingConstants.EnableQuickSyncDecoding, this.EnableQuickSyncDecoding);
            this.userSettingService.SetUserSetting(UserSettingConstants.ScalingMode, this.SelectedScalingMode);
            this.userSettingService.SetUserSetting(UserSettingConstants.UseQSVDecodeForNonQSVEnc, this.UseQSVDecodeForNonQSVEnc);

            this.userSettingService.SetUserSetting(UserSettingConstants.EnableQuickSyncEncoding, this.EnableQuickSyncEncoding);
            this.userSettingService.SetUserSetting(UserSettingConstants.EnableVceEncoder, this.EnableVceEncoder);
            this.userSettingService.SetUserSetting(UserSettingConstants.EnableNvencEncoder, this.EnableNvencEncoder);

            /* System and Logging */
            userSettingService.SetUserSetting(UserSettingConstants.ProcessPriority, this.SelectedPriority);
            userSettingService.SetUserSetting(UserSettingConstants.PreventSleep, this.PreventSleep);
            userSettingService.SetUserSetting(UserSettingConstants.PauseOnLowDiskspace, this.PauseOnLowDiskspace);
            userSettingService.SetUserSetting(UserSettingConstants.PauseOnLowDiskspaceLevel, this.PauseOnLowDiskspaceLevel);
            userSettingService.SetUserSetting(UserSettingConstants.Verbosity, this.SelectedVerbosity);
            userSettingService.SetUserSetting(UserSettingConstants.SaveLogWithVideo, this.CopyLogToEncodeDirectory);
            userSettingService.SetUserSetting(UserSettingConstants.SaveLogToCopyDirectory, this.CopyLogToSepcficedLocation);
            userSettingService.SetUserSetting(UserSettingConstants.SaveLogCopyDirectory, this.LogDirectory);
            userSettingService.SetUserSetting(UserSettingConstants.ClearOldLogs, this.ClearOldOlgs);

            /* Advanced */
            userSettingService.SetUserSetting(UserSettingConstants.MainWindowMinimize, this.MinimiseToTray);
            userSettingService.SetUserSetting(UserSettingConstants.ClearCompletedFromQueue, this.ClearQueueOnEncodeCompleted);
            userSettingService.SetUserSetting(UserSettingConstants.PreviewScanCount, this.SelectedPreviewCount);
            userSettingService.SetUserSetting(UserSettingConstants.X264Step, double.Parse(this.SelectedGranulairty, CultureInfo.InvariantCulture));
            userSettingService.SetUserSetting(UserSettingConstants.ShowAdvancedTab, this.ShowAdvancedTab);

            int value;
            if (int.TryParse(this.MinLength.ToString(CultureInfo.InvariantCulture), out value))
            {
                this.userSettingService.SetUserSetting(UserSettingConstants.MinScanDuration, value);
            }

            userSettingService.SetUserSetting(UserSettingConstants.DisableLibDvdNav, this.DisableLibdvdNav);
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
                this.UpdateMessage = Resources.OptionsViewModel_NewUpdate;
                this.UpdateAvailable = true;
            }
            else if (Environment.Is64BitOperatingSystem && !System.Environment.Is64BitProcess)
            {
                this.UpdateMessage = Resources.OptionsViewModel_64bitAvailable;
                this.UpdateAvailable = true;
            }
            else
            {
                this.UpdateMessage = Resources.OptionsViewModel_NoNewUpdates;
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
                this.UpdateMessage = info.WasSuccessful ? Resources.OptionsViewModel_UpdateDownloaded : Resources.OptionsViewModel_UpdateServiceUnavailable;
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
            this.UpdateMessage = info.WasSuccessful ? Resources.OptionsViewModel_UpdateDownloaded : info.Message;

            if (info.WasSuccessful)
            {
                Process.Start(Path.Combine(Path.GetTempPath(), "handbrake-setup.exe"));
                Execute.OnUIThread(() => Application.Current.Shutdown());
            }
        }

        /// <summary>
        /// Validate the Autoname Fileformat string
        /// </summary>
        /// <param name="input">The format string</param>
        /// <param name="isSilent">Don't show an error dialog if true.</param>
        /// <returns>True if valid</returns>
        private bool IsValidAutonameFormat(string input, bool isSilent)
        {
            char[] invalidchars = Path.GetInvalidFileNameChars();
            Array.Sort(invalidchars);
            foreach (var characterToTest in input)
            {
                if (Array.BinarySearch(invalidchars, characterToTest) >= 0)
                {
                    if (!isSilent)
                    {
                        this.errorService.ShowMessageBox(
                            ResourcesUI.OptionsView_InvalidFileFormatChars,
                            Resources.Error,
                            MessageBoxButton.OK,
                            MessageBoxImage.Error);
                    }
                    return false;
                }
            }

            return true;
        }
    }
}