// --------------------------------------------------------------------------------------------------------------------
// <copyright file="OptionsViewModelBase.cs" company="HandBrake Project (http://handbrake.fr)">
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
    using System.Threading.Tasks;
    using Caliburn.Micro;
    using HandBrake;
    using HandBrake.CoreLibrary.Model;
    using HandBrake.CoreLibrary.Utilities;
    using HandBrake.Model.Prompts;
    using HandBrake.Utilities.Interfaces;
    using HandBrakeWPF.Model;
    using HandBrakeWPF.Properties;
    using HandBrakeWPF.Services.Interfaces;
    using HandBrakeWPF.Utilities;
    using HandBrakeWPF.ViewModels.Interfaces;
    using PlatformBindings;
    using PlatformBindings.Models.FileSystem;
    using PlatformBindings.Services;
    using SystemInfo = HandBrake.CoreLibrary.Utilities.SystemInfo;

    /// <summary>
    /// The Options View Model
    /// </summary>
    public abstract class OptionsViewModelBase : ViewModelBase, IOptionsViewModel
    {
        #region Constants and Fields

        protected readonly IUserSettingService UserSettingService;
        protected readonly IErrorService ErrorService;
        protected readonly IDialogService DialogService;
        protected readonly IOBindings IOService;

        private string arguments;
        private string autoNameDefaultPath;
        private string originalAutoNameDefaultPath;
        private bool automaticallyNameFiles;
        private string autonameFormat;
        private bool changeToTitleCase;
        private bool clearOldOlgs;
        private BindingList<string> constantQualityGranularity = new BindingList<string>();
        private bool copyLogToEncodeDirectory;
        private bool copyLogToSepcficedLocation;
        private bool disableLibdvdNav;
        private string logDirectory;
        private string originalLogDirectory;
        private BindingList<int> logVerbosityOptions = new BindingList<int>();
        private long minLength;
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
        private string originalSendFileToPath;
        private string whenDone;
        private BindingList<string> whenDoneOptions = new BindingList<string>();
        private bool clearQueueOnEncodeCompleted;
        private OptionsTab selectedTab;
        private bool showAdvancedTab;
        private bool removePunctuation;
        private bool resetWhenDoneAction;

        private bool enableQuickSyncDecoding;
        private bool showQueueInline;
        private bool pauseOnLowDiskspace;
        private long pauseOnLowDiskspaceLevel;
        private bool useQsvDecodeForNonQsvEnc;
        private bool showStatusInTitleBar;

        private string whenDoneAudioFile;
        private string originalWhenDoneAudoFile;
        private bool playSoundWhenDone;
        private bool playSoundWhenQueueDone;

        #endregion Constants and Fields

        #region Constructors and Destructors

        /// <summary>
        /// Initializes a new instance of the <see cref="OptionsViewModelBase"/> class.
        /// </summary>
        /// <param name="userSettingService">
        /// The user Setting Service.
        /// </param>
        /// <param name="aboutViewModel">
        /// The about View Model.
        /// </param>
        /// <param name="errorService">
        /// The error Service.
        /// </param>
        public OptionsViewModelBase(IUserSettingService userSettingService, IAboutViewModel aboutViewModel, IErrorService errorService)
        {
            this.Title = "Options";
            this.UserSettingService = userSettingService;
            this.ErrorService = errorService;
            this.DialogService = HandBrakeServices.Current.Dialog;
            this.IOService = AppServices.Current?.IO;
            this.AboutViewModel = aboutViewModel;
            this.OnLoad();

            this.SelectedTab = OptionsTab.General;
        }

        #endregion Constructors and Destructors

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

        #endregion Window Properties

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

        #endregion General

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

        #endregion Output Files

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

        #endregion System and Logging

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

        #endregion Advanced

        #region Video

        /// <summary>
        /// Gets or sets a value indicating whether disable quick sync decoding.
        /// </summary>
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

        /// <summary>
        /// Gets or sets the selected scaling mode.
        /// </summary>
        public VideoScaler SelectedScalingMode { get; set; }

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
        /// Gets a value indicating whether is use qsv dec available.
        /// </summary>
        public bool IsUseQsvDecAvailable
        {
            get
            {
                return IsQuickSyncAvailable && this.EnableQuickSyncDecoding;
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether to use qsv decode for non qsv encoders
        /// </summary>
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

        #endregion Video

        #endregion Properties

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

        #endregion About HandBrake

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
            var file = IOService?.Pickers?.PickFile()?.Result;
            if (file != null)
            {
                this.SendFileTo = Path.GetFileNameWithoutExtension(file.Path);
                this.sendFileToPath = file.Path;
            }
        }

        /// <summary>
        /// Browse Auto Name Path
        /// </summary>
        public void BrowseAutoNamePath()
        {
            var folder = IOService?.Pickers?.PickFolder()?.Result;
            if (folder != null)
            {
                this.AutoNameDefaultPath = folder.Path;
            }
        }

        /// <summary>
        /// Browse - Log Path
        /// </summary>
        public void BrowseLogPath()
        {
            var properties = new FolderPickerProperties();
            var suggestedFolder = AppServices.Current?.IO?.GetFolder(this.logDirectory)?.Result;
            // properties.SuggestedStorageItem = suggestedFolder; Not Ready yet.

            var folder = IOService?.Pickers?.PickFolder(properties)?.Result;
            if (folder != null)
            {
                this.LogDirectory = folder.Path;
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
            var result = this.DialogService.Show("Are you sure you wish to clear the log file directory?", "Clear Logs",
                                                  DialogButtonType.YesNoCancel, DialogType.Question);
            if (result == DialogResult.Yes)
            {
                GeneralUtilities.ClearLogFiles(0);
                this.DialogService.Show("HandBrake's Log file directory has been cleared!", "Notice", DialogButtonType.OK, DialogType.Information);
            }
        }

        /// <summary>
        /// Browse - Send File To
        /// </summary>
        public void BrowseWhenDoneAudioFile()
        {
            var properties = new FilePickerProperties();
            properties.FileTypes.Add(".wav");
            properties.FileTypes.Add(".mp3");
            var suggestedFile = AppServices.Current?.IO?.GetFile(this.WhenDoneAudioFileFullPath)?.Result;
            // properties.SuggestedStorageItem = suggestedFile; Not Ready yet.

            var file = IOService?.Pickers?.PickFile(properties)?.Result;
            if (file != null)
            {
                this.WhenDoneAudioFile = Path.GetFileNameWithoutExtension(file.Path);
                this.WhenDoneAudioFileFullPath = file.Path;
            }
        }

        #endregion Public Methods

        /// <summary>
        /// Load User Settings
        /// </summary>
        public override void OnLoad()
        {
            // #############################
            // General
            // #############################

            // On Encode Completeion Action
            this.whenDoneOptions.Clear();
            this.whenDoneOptions.Add("Do nothing");
            this.whenDoneOptions.Add("Shutdown");
            this.whenDoneOptions.Add("Suspend");
            this.whenDoneOptions.Add("Hibernate");
            this.whenDoneOptions.Add("Lock System");
            this.whenDoneOptions.Add("Log off");
            this.whenDoneOptions.Add("Quit HandBrake");
            this.WhenDone = UserSettingService.GetUserSetting<string>("WhenCompleteAction");
            if (this.UserSettingService.GetUserSetting<bool>(UserSettingConstants.ResetWhenDoneAction))
            {
                this.WhenDone = "Do nothing";
            }

            this.SendFileAfterEncode = this.UserSettingService.GetUserSetting<bool>(UserSettingConstants.SendFile);
            this.SendFileTo = Path.GetFileNameWithoutExtension(this.UserSettingService.GetUserSetting<string>(UserSettingConstants.SendFileTo)) ?? string.Empty;
            this.SendFileToPath = this.UserSettingService.GetUserSetting<string>(UserSettingConstants.SendFileTo) ?? string.Empty;
            this.originalSendFileToPath = this.sendFileToPath;
            this.Arguments = this.UserSettingService.GetUserSetting<string>(UserSettingConstants.SendFileToArgs) ?? string.Empty;
            this.ResetWhenDoneAction = this.UserSettingService.GetUserSetting<bool>(UserSettingConstants.ResetWhenDoneAction);
            this.ShowQueueInline = this.UserSettingService.GetUserSetting<bool>(UserSettingConstants.ShowQueueInline);
            this.ShowStatusInTitleBar = this.UserSettingService.GetUserSetting<bool>(UserSettingConstants.ShowStatusInTitleBar);
            this.WhenDoneAudioFile = Path.GetFileNameWithoutExtension(this.UserSettingService.GetUserSetting<string>(UserSettingConstants.WhenDoneAudioFile)) ?? string.Empty;
            this.originalWhenDoneAudoFile = this.whenDoneAudioFile;
            this.WhenDoneAudioFileFullPath = this.UserSettingService.GetUserSetting<string>(UserSettingConstants.WhenDoneAudioFile);
            this.PlaySoundWhenDone = this.UserSettingService.GetUserSetting<bool>(UserSettingConstants.PlaySoundWhenDone);
            this.PlaySoundWhenQueueDone = this.UserSettingService.GetUserSetting<bool>(UserSettingConstants.PlaySoundWhenQueueDone);

            // #############################
            // Output Settings
            // #############################

            // Enable auto naming feature.)
            this.AutomaticallyNameFiles = this.UserSettingService.GetUserSetting<bool>(UserSettingConstants.AutoNaming);

            // Store the auto name path
            this.AutoNameDefaultPath = this.UserSettingService.GetUserSetting<string>(UserSettingConstants.AutoNamePath) ?? string.Empty;
            this.originalAutoNameDefaultPath = this.autoNameDefaultPath;
            if (string.IsNullOrEmpty(this.autoNameDefaultPath))
                this.AutoNameDefaultPath = "Click 'Browse' to set the default location";

            // Store auto name format
            string anf = this.UserSettingService.GetUserSetting<string>(UserSettingConstants.AutoNameFormat) ?? string.Empty;
            this.AutonameFormat = this.IsValidAutonameFormat(anf, true) ? anf : "{source}-{title}";

            // Use iPod/iTunes friendly .m4v extension for MP4 files.
            this.mp4ExtensionOptions.Clear();
            this.mp4ExtensionOptions.Add("Automatic");
            this.mp4ExtensionOptions.Add("Always use MP4");
            this.mp4ExtensionOptions.Add("Always use M4V");
            this.SelectedMp4Extension = this.UserSettingService.GetUserSetting<int>(UserSettingConstants.UseM4v);

            // Remove Underscores
            this.RemoveUnderscores = this.UserSettingService.GetUserSetting<bool>(UserSettingConstants.AutoNameRemoveUnderscore);

            // Title case
            this.ChangeToTitleCase = this.UserSettingService.GetUserSetting<bool>(UserSettingConstants.AutoNameTitleCase);
            this.RemovePunctuation = this.UserSettingService.GetUserSetting<bool>(UserSettingConstants.RemovePunctuation);

            // #############################
            // Video
            // #############################
            this.EnableQuickSyncDecoding = this.UserSettingService.GetUserSetting<bool>(UserSettingConstants.EnableQuickSyncDecoding);
            this.SelectedScalingMode = this.UserSettingService.GetUserSetting<VideoScaler>(UserSettingConstants.ScalingMode);
            this.UseQSVDecodeForNonQSVEnc = this.UserSettingService.GetUserSetting<bool>(UserSettingConstants.UseQSVDecodeForNonQSVEnc);

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
            this.SelectedPriority = UserSettingService.GetUserSetting<string>(UserSettingConstants.ProcessPriority);

            this.PreventSleep = UserSettingService.GetUserSetting<bool>(UserSettingConstants.PreventSleep);
            this.PauseOnLowDiskspace = UserSettingService.GetUserSetting<bool>(UserSettingConstants.PauseOnLowDiskspace);
            this.PauseOnLowDiskspaceLevel = this.UserSettingService.GetUserSetting<long>(UserSettingConstants.PauseOnLowDiskspaceLevel);

            // Log Verbosity Level
            this.logVerbosityOptions.Clear();
            this.logVerbosityOptions.Add(0);
            this.logVerbosityOptions.Add(1);
            this.logVerbosityOptions.Add(2);
            this.SelectedVerbosity = UserSettingService.GetUserSetting<int>(UserSettingConstants.Verbosity);

            // Logs
            this.CopyLogToEncodeDirectory = UserSettingService.GetUserSetting<bool>(UserSettingConstants.SaveLogWithVideo);
            this.CopyLogToSepcficedLocation = UserSettingService.GetUserSetting<bool>(UserSettingConstants.SaveLogToCopyDirectory);

            // The saved log path
            this.LogDirectory = UserSettingService.GetUserSetting<string>(UserSettingConstants.SaveLogCopyDirectory) ?? string.Empty;
            this.originalLogDirectory = this.logDirectory;

            this.ClearOldOlgs = this.UserSettingService.GetUserSetting<bool>(UserSettingConstants.ClearOldLogs);

            // #############################
            // Advanced
            // #############################

            this.ClearQueueOnEncodeCompleted = UserSettingService.GetUserSetting<bool>(UserSettingConstants.ClearCompletedFromQueue);
            this.ShowAdvancedTab = UserSettingService.GetUserSetting<bool>(UserSettingConstants.ShowAdvancedTab);

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
            this.SelectedPreviewCount = this.UserSettingService.GetUserSetting<int>(UserSettingConstants.PreviewScanCount);

            // x264 step
            this.ConstantQualityGranularity.Clear();
            this.ConstantQualityGranularity.Add("1.00");
            this.ConstantQualityGranularity.Add("0.50");
            this.ConstantQualityGranularity.Add("0.25");
            this.SelectedGranulairty = UserSettingService.GetUserSetting<double>(UserSettingConstants.X264Step).ToString("0.00", CultureInfo.InvariantCulture);

            // Min Title Length
            this.MinLength = this.UserSettingService.GetUserSetting<int>(UserSettingConstants.MinScanDuration);

            // Use dvdnav
            this.DisableLibdvdNav = UserSettingService.GetUserSetting<bool>(UserSettingConstants.DisableLibDvdNav);
        }

        /// <summary>
        /// Some settings can be changed outside of this window. This will refresh their UI controls.
        /// </summary>
        public void UpdateSettings()
        {
            this.WhenDone = UserSettingService.GetUserSetting<string>("WhenCompleteAction");
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
        protected virtual void Save()
        {
            /* General */
            this.UserSettingService.SetUserSetting(UserSettingConstants.WhenCompleteAction, this.WhenDone);
            this.UserSettingService.SetUserSetting(UserSettingConstants.SendFileTo, this.SendFileToPath);
            this.UserSettingService.SetUserSetting(UserSettingConstants.SendFile, this.SendFileAfterEncode);
            this.UserSettingService.SetUserSetting(UserSettingConstants.SendFileToArgs, this.Arguments);
            this.UserSettingService.SetUserSetting(UserSettingConstants.ResetWhenDoneAction, this.ResetWhenDoneAction);
            this.UserSettingService.SetUserSetting(UserSettingConstants.ShowQueueInline, this.ShowQueueInline);
            this.UserSettingService.SetUserSetting(UserSettingConstants.ShowStatusInTitleBar, this.ShowStatusInTitleBar);
            this.UserSettingService.SetUserSetting(UserSettingConstants.PlaySoundWhenDone, this.PlaySoundWhenDone);
            this.UserSettingService.SetUserSetting(UserSettingConstants.PlaySoundWhenQueueDone, this.PlaySoundWhenQueueDone);
            this.UserSettingService.SetUserSetting(UserSettingConstants.WhenDoneAudioFile, this.WhenDoneAudioFileFullPath);

            /* Output Files */
            this.UserSettingService.SetUserSetting(UserSettingConstants.AutoNaming, this.AutomaticallyNameFiles);
            this.UserSettingService.SetUserSetting(UserSettingConstants.AutoNameFormat, this.AutonameFormat);
            this.UserSettingService.SetUserSetting(UserSettingConstants.AutoNamePath, this.AutoNameDefaultPath);
            this.UserSettingService.SetUserSetting(UserSettingConstants.UseM4v, this.SelectedMp4Extension);
            this.UserSettingService.SetUserSetting(UserSettingConstants.AutoNameRemoveUnderscore, this.RemoveUnderscores);
            this.UserSettingService.SetUserSetting(UserSettingConstants.AutoNameTitleCase, this.ChangeToTitleCase);
            this.UserSettingService.SetUserSetting(UserSettingConstants.RemovePunctuation, this.RemovePunctuation);

            /* Video */
            this.UserSettingService.SetUserSetting(UserSettingConstants.EnableQuickSyncDecoding, this.EnableQuickSyncDecoding);
            this.UserSettingService.SetUserSetting(UserSettingConstants.ScalingMode, this.SelectedScalingMode);
            this.UserSettingService.SetUserSetting(UserSettingConstants.UseQSVDecodeForNonQSVEnc, this.UseQSVDecodeForNonQSVEnc);

            /* System and Logging */
            UserSettingService.SetUserSetting(UserSettingConstants.ProcessPriority, this.SelectedPriority);
            UserSettingService.SetUserSetting(UserSettingConstants.PreventSleep, this.PreventSleep);
            UserSettingService.SetUserSetting(UserSettingConstants.PauseOnLowDiskspace, this.PauseOnLowDiskspace);
            UserSettingService.SetUserSetting(UserSettingConstants.PauseOnLowDiskspaceLevel, this.PauseOnLowDiskspaceLevel);
            UserSettingService.SetUserSetting(UserSettingConstants.Verbosity, this.SelectedVerbosity);
            UserSettingService.SetUserSetting(UserSettingConstants.SaveLogWithVideo, this.CopyLogToEncodeDirectory);
            UserSettingService.SetUserSetting(UserSettingConstants.SaveLogToCopyDirectory, this.CopyLogToSepcficedLocation);
            UserSettingService.SetUserSetting(UserSettingConstants.SaveLogCopyDirectory, this.LogDirectory);
            UserSettingService.SetUserSetting(UserSettingConstants.ClearOldLogs, this.ClearOldOlgs);

            /* Advanced */
            UserSettingService.SetUserSetting(UserSettingConstants.ClearCompletedFromQueue, this.ClearQueueOnEncodeCompleted);
            UserSettingService.SetUserSetting(UserSettingConstants.PreviewScanCount, this.SelectedPreviewCount);
            UserSettingService.SetUserSetting(UserSettingConstants.X264Step, double.Parse(this.SelectedGranulairty, CultureInfo.InvariantCulture));
            UserSettingService.SetUserSetting(UserSettingConstants.ShowAdvancedTab, this.ShowAdvancedTab);

            int value;
            if (int.TryParse(this.MinLength.ToString(CultureInfo.InvariantCulture), out value))
            {
                this.UserSettingService.SetUserSetting(UserSettingConstants.MinScanDuration, value);
            }

            UserSettingService.SetUserSetting(UserSettingConstants.DisableLibDvdNav, this.DisableLibdvdNav);

            // Get File/Folder tokens for future access if required.
            if (IOService.FutureAccess != null)
            {
                Task.Run(() =>
                {
                    var settings = AppServices.Current.IO.LocalSettings;

                    if (this.logDirectory != this.originalLogDirectory)
                    {
                        try
                        {
                            // Remove old Token.
                            try
                            {
                                var oldToken = settings.GetValue<string>(UserSettingConstants.SaveLogCopyDirectory);
                                IOService.FutureAccess.RemoveFutureAccessPermission(oldToken);
                            }
                            catch { }

                            var newfile = IOService.GetFolder(this.LogDirectory).Result;
                            var token = IOService.FutureAccess.GetFutureAccessPermission(newfile);
                            settings.SetValue(UserSettingConstants.SaveLogCopyDirectory, token);
                        }
                        catch
                        {
                        }
                    }

                    if (this.sendFileToPath != this.originalSendFileToPath)
                    {
                        try
                        {
                            // Remove old Token.
                            try
                            {
                                var oldToken = settings.GetValue<string>(UserSettingConstants.SendFileTo);
                                IOService.FutureAccess.RemoveFutureAccessPermission(oldToken);
                            }
                            catch { }

                            var newfile = IOService.GetFile(this.sendFileToPath).Result;
                            var token = IOService.FutureAccess.GetFutureAccessPermission(newfile);
                            settings.SetValue(UserSettingConstants.SendFileTo, token);
                        }
                        catch
                        {
                        }
                    }

                    if (this.whenDoneAudioFile != this.originalWhenDoneAudoFile)
                    {
                        try
                        {
                            // Remove old Token.
                            try
                            {
                                var oldToken = settings.GetValue<string>(UserSettingConstants.WhenDoneAudioFile);
                                IOService.FutureAccess.RemoveFutureAccessPermission(oldToken);
                            }
                            catch { }

                            var newfile = IOService.GetFile(this.whenDoneAudioFile).Result;
                            var token = IOService.FutureAccess.GetFutureAccessPermission(newfile);
                            settings.SetValue(UserSettingConstants.WhenDoneAudioFile, token);
                        }
                        catch
                        {
                        }
                    }

                    if (this.autoNameDefaultPath != this.originalAutoNameDefaultPath)
                    {
                        try
                        {
                            // Remove old Token.
                            try
                            {
                                var oldToken = settings.GetValue<string>(UserSettingConstants.AutoNamePath);
                                IOService.FutureAccess.RemoveFutureAccessPermission(oldToken);
                            }
                            catch { }

                            var newfolder = IOService.GetFolder(this.autoNameDefaultPath).Result;
                            var token = IOService.FutureAccess.GetFutureAccessPermission(newfolder);
                            settings.SetValue(UserSettingConstants.AutoNamePath, token);
                        }
                        catch
                        {
                        }
                    }
                });
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
                        this.ErrorService.ShowMessageBox(
                            ResourcesUI.OptionsView_InvalidFileFormatChars,
                            Resources.Error,
                            DialogButtonType.OK,
                            DialogType.Error);
                    }
                    return false;
                }
            }

            return true;
        }
    }
}