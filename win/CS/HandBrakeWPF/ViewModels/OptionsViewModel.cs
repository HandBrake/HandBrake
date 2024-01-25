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
    using System.ComponentModel;
    using System.Diagnostics;
    using System.Globalization;
    using System.IO;
    using System.Linq;
    using System.Media;
    using System.Windows;
    using System.Windows.Documents;
    using System.Windows.Media;

    using HandBrake.App.Core.Utilities;
    using HandBrake.Interop.Interop;

    using HandBrakeWPF.Commands;
    using HandBrakeWPF.Helpers;
    using HandBrakeWPF.Model;
    using HandBrakeWPF.Model.Options;
    using HandBrakeWPF.Model.Video;
    using HandBrakeWPF.Properties;
    using HandBrakeWPF.Services;
    using HandBrakeWPF.Services.Interfaces;
    using HandBrakeWPF.Services.Presets.Interfaces;
    using HandBrakeWPF.Utilities;
    using HandBrakeWPF.Utilities.FileDialogs;
    using HandBrakeWPF.ViewModels.Interfaces;

    using ILog = HandBrakeWPF.Services.Logging.Interfaces.ILog;
    public class OptionsViewModel : ViewModelBase, IOptionsViewModel
    {
        private readonly IUserSettingService userSettingService;
        private readonly IUpdateService updateService;
        private readonly IErrorService errorService;
        private readonly IPresetService presetService;

        private readonly INotificationService notificationService;

        private readonly ILog logService;

        private string arguments;
        private string autoNameDefaultPath;
        private bool automaticallyNameFiles;
        private string autonameFormat;
        private bool changeToTitleCase;
        private bool checkForUpdates;
        private UpdateCheck checkForUpdatesFrequency;
        private bool clearOldLogs;
        private BindingList<string> constantQualityGranularity = new BindingList<string>();
        private bool copyLogToEncodeDirectory;
        private bool copyLogToSpecifiedLocation;
        private bool disableLibdvdNav;
        private string logDirectory;
        private BindingList<int> logVerbosityOptions = new BindingList<int>();
        private long minLength;
        private bool minimiseToTray;
        private bool preventSleep;
        private BindingList<int> previewPicturesToScan = new BindingList<int>();
        private bool removeUnderscores;
        private string selectedGranularity;
        private Mp4Behaviour selectedMp4Extension;
        private int selectedPreviewCount;
        private ProcessPriority selectedPriority;
        private int selectedVerbosity;
        private bool sendFileAfterEncode;
        private string sendFileTo;
        private string sendFileToPath;
        private string vlcPath;
        private WhenDone whenDone;
        private bool clearQueueOnEncodeCompleted;
        private OptionsTab selectedTab;
        private string updateMessage;
        private bool updateAvailable;
        private int downloadProgressPercentage;
        private UpdateCheckInformation updateInfo;
        private bool removePunctuation;
        private bool resetWhenDoneAction;
        private bool enableQuickSyncDecoding;
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
        private InterfaceLanguage selectedLanguage;
        private bool showAddSelectionToQueue;
        private bool showAddAllToQueue;
        private int selectedOverwriteBehaviour;
        private int selectedCollisionBehaviour;
        private string prePostFilenameText;
        private bool showPrePostFilenameBox;
        private bool whenDonePerformActionImmediately;
        private DarkThemeMode darkThemeMode;
        private bool alwaysUseDefaultPath;
        private bool pauseOnLowBattery;
        private int lowBatteryLevel;
        private string whenDoneAudioFileFullPath;
        private PresetDisplayMode selectedPresetDisplayMode;
        private int remoteServicePort;
        private bool remoteServiceEnabled;
        private bool enableQuickSyncLowPower;
        private int simultaneousEncodes;
        private bool enableQuickSyncHyperEncode;
        private bool enableNvDecSupport;
        private bool useIsoDateFormat;
        private BindingList<string> excludedFileExtensions;
        private bool recursiveFolderScan;

        public OptionsViewModel(
            IUserSettingService userSettingService,
            IUpdateService updateService,
            IAboutViewModel aboutViewModel,
            IErrorService errorService,
            IPresetService presetService,
            INotificationService notificationService,
            ILog logService)
        {
            this.Title = "Options";
            this.userSettingService = userSettingService;
            this.updateService = updateService;
            this.errorService = errorService;
            this.presetService = presetService;
            this.notificationService = notificationService;
            this.logService = logService;
            this.AboutViewModel = aboutViewModel;
            this.OnLoad();

            this.SelectedTab = OptionsTab.General;
            this.UpdateMessage = Resources.OptionsViewModel_CheckForUpdatesMsg;
            this.RemoveExtensionCommand = new SimpleRelayCommand<string>(this.RemoveExcludedExtension);
        }

        public OptionsTab SelectedTab
        {
            get => this.selectedTab;

            set
            {
                this.selectedTab = value;
                this.NotifyOfPropertyChange(() => this.SelectedTab);
            }
        }

        public IAboutViewModel AboutViewModel { get; set; }

        public bool IsNightly { get; } = HandBrakeVersionHelper.IsNightly();

        public bool HasSystemBattery => PowerService.HasBattery();

        /* General */

        public BindingList<InterfaceLanguage> InterfaceLanguages { get; } = new BindingList<InterfaceLanguage>(InterfaceLanguageUtilities.GetUserInterfaceLanguages());

        public InterfaceLanguage SelectedLanguage
        {
            get => this.selectedLanguage;
            set
            {
                if (Equals(value, this.selectedLanguage)) return;
                this.selectedLanguage = value;
                this.NotifyOfPropertyChange(() => this.SelectedLanguage);
            }
        }

        public BindingList<RightToLeftMode> RightToLeftModes { get; } = new BindingList<RightToLeftMode>(EnumHelper<RightToLeftMode>.GetEnumList().ToList());

        public RightToLeftMode SelectedRightToLeftMode { get;set; }

        public bool CheckForUpdates
        {
            get => this.checkForUpdates;

            set
            {
                this.checkForUpdates = value;
                this.NotifyOfPropertyChange(() => this.CheckForUpdates);
            }
        }

        public bool CheckForUpdatesAllowed { get; set; }

        public bool ResetWhenDoneAction
        {
            get => this.resetWhenDoneAction;

            set
            {
                this.resetWhenDoneAction = value;
                this.NotifyOfPropertyChange(() => this.ResetWhenDoneAction);
            }
        }

        public BindingList<UpdateCheck> CheckForUpdatesFrequencies { get; } = new BindingList<UpdateCheck>(EnumHelper<UpdateCheck>.GetEnumList().ToList());

        public UpdateCheck CheckForUpdatesFrequency
        {
            get => this.checkForUpdatesFrequency;

            set
            {
                this.checkForUpdatesFrequency = value;
                this.NotifyOfPropertyChange(() => this.CheckForUpdatesFrequency);
            }
        }

        public bool WhenDonePerformActionImmediately
        {
            get => this.whenDonePerformActionImmediately;
            set
            {
                if (value == this.whenDonePerformActionImmediately) return;
                this.whenDonePerformActionImmediately = value;
                this.NotifyOfPropertyChange(() => this.WhenDonePerformActionImmediately);
            }
        }

        public bool ShowStatusInTitleBar
        {
            get => this.showStatusInTitleBar;
            set
            {
                if (value == this.showStatusInTitleBar) return;
                this.showStatusInTitleBar = value;
                this.NotifyOfPropertyChange(() => this.ShowStatusInTitleBar);
            }
        }

        public bool ShowPreviewOnSummaryTab
        {
            get => this.showPreviewOnSummaryTab;
            set
            {
                if (value == this.showPreviewOnSummaryTab) return;
                this.showPreviewOnSummaryTab = value;
                this.NotifyOfPropertyChange(() => this.ShowPreviewOnSummaryTab);
            }
        }

        public bool ShowAddSelectionToQueue
        {
            get => this.showAddSelectionToQueue;
            set
            {
                if (value == this.showAddSelectionToQueue) return;
                this.showAddSelectionToQueue = value;
                this.NotifyOfPropertyChange(() => this.ShowAddSelectionToQueue);
            }
        }

        public bool ShowAddAllToQueue
        {
            get => this.showAddAllToQueue;
            set
            {
                if (value == this.showAddAllToQueue) return;
                this.showAddAllToQueue = value;
                this.NotifyOfPropertyChange(() => this.ShowAddAllToQueue);
            }
        }

        public BindingList<DarkThemeMode> DarkThemeModes { get; } = new BindingList<DarkThemeMode>(EnumHelper<DarkThemeMode>.GetEnumList().ToList());

        public DarkThemeMode DarkThemeMode
        {
            get => this.darkThemeMode;
            set
            {
                if (value == this.darkThemeMode) return;
                this.darkThemeMode = value;
                this.NotifyOfPropertyChange(() => this.DarkThemeMode);
            }
        }

        public BindingList<PresetDisplayMode> PresetDisplayModes { get; } = new BindingList<PresetDisplayMode>(EnumHelper<PresetDisplayMode>.GetEnumList().ToList());

        public PresetDisplayMode SelectedPresetDisplayMode
        {
            get => this.selectedPresetDisplayMode;
            set
            {
                if (value == this.selectedPresetDisplayMode) return;
                this.selectedPresetDisplayMode = value;
                this.NotifyOfPropertyChange(() => this.SelectedPresetDisplayMode);
            }
        }

        /* When Done */

        public string SendFileTo
        {
            get => this.sendFileTo;

            set
            {
                this.sendFileTo = value;
                this.NotifyOfPropertyChange(() => this.SendFileTo);
            }
        }

        public string SendFileToPath
        {
            get => this.sendFileToPath;

            set
            {
                this.sendFileToPath = value;
                this.NotifyOfPropertyChange(() => this.SendFileToPath);
            }
        }

        public string Arguments
        {
            get => this.arguments;

            set
            {
                this.arguments = value;
                this.NotifyOfPropertyChange(() => this.Arguments);
            }
        }

        public bool PlaySoundWhenDone
        {
            get => this.playSoundWhenDone;
            set
            {
                if (value == this.playSoundWhenDone) return;
                this.playSoundWhenDone = value;
                this.NotifyOfPropertyChange(() => this.PlaySoundWhenDone);
            }
        }

        public bool PlaySoundWhenQueueDone
        {
            get => this.playSoundWhenQueueDone;
            set
            {
                if (value == this.playSoundWhenQueueDone) return;
                this.playSoundWhenQueueDone = value;
                this.NotifyOfPropertyChange(() => this.PlaySoundWhenQueueDone);
            }
        }

        public string WhenDoneAudioFile
        {
            get => this.whenDoneAudioFile;
            set
            {
                if (value == this.whenDoneAudioFile) return;
                this.whenDoneAudioFile = value;
                this.NotifyOfPropertyChange(() => this.WhenDoneAudioFile);
            }
        }

        public string WhenDoneAudioFileFullPath
        {
            get => this.whenDoneAudioFileFullPath;
            set
            {
                if (value == this.whenDoneAudioFileFullPath) return;
                this.whenDoneAudioFileFullPath = value;
                this.NotifyOfPropertyChange(() => this.WhenDoneAudioFileFullPath);
            }
        }

        public bool SendFileAfterEncode
        {
            get => this.sendFileAfterEncode;

            set
            {
                this.sendFileAfterEncode = value;
                this.NotifyOfPropertyChange(() => this.SendFileAfterEncode);
            }
        }


        public WhenDone WhenDone
        {
            get => this.whenDone;

            set
            {
                this.whenDone = value;
                this.NotifyOfPropertyChange(() => this.WhenDone);
            }
        }

        public BindingList<WhenDone> WhenDoneOptions { get; } = new BindingList<WhenDone>(EnumHelper<WhenDone>.GetEnumList().ToList());

        public bool SendSystemNotificationOnEncodeDone { get; set; }

        public bool SendSystemNotificationOnQueueDone { get; set; }

        /* Output Files */

        public string AutoNameDefaultPath
        {
            get => this.autoNameDefaultPath;

            set
            {
                if (this.IsAutonamePathValid(value))
                {
                    this.autoNameDefaultPath = value;
                    this.NotifyOfPropertyChange(() => this.AutoNameDefaultPath);
                }
            }
        }

        public bool AutomaticallyNameFiles
        {
            get => this.automaticallyNameFiles;

            set
            {
                this.automaticallyNameFiles = value;
                this.NotifyOfPropertyChange(() => this.AutomaticallyNameFiles);
            }
        }

        public string AutonameFormat
        {
            get => this.autonameFormat;

            set
            {
                if (this.IsValidAutonameFormat(value, false))
                {
                    this.autonameFormat = value;
                }

                this.NotifyOfPropertyChange(() => this.AutonameFormat);
            }
        }

        public bool ChangeToTitleCase
        {
            get => this.changeToTitleCase;

            set
            {
                this.changeToTitleCase = value;
                this.NotifyOfPropertyChange(() => this.ChangeToTitleCase);
            }
        }

        public bool RemoveUnderscores
        {
            get => this.removeUnderscores;

            set
            {
                this.removeUnderscores = value;
                this.NotifyOfPropertyChange(() => this.RemoveUnderscores);
            }
        }

        public BindingList<Mp4Behaviour> Mp4ExtensionOptions { get; } = new BindingList<Mp4Behaviour>(EnumHelper<Mp4Behaviour>.GetEnumList().ToList());

        public Mp4Behaviour SelectedMp4Extension
        {
            get => this.selectedMp4Extension;

            set
            {
                this.selectedMp4Extension = value;
                this.NotifyOfPropertyChange(() => this.SelectedMp4Extension);
            }
        }

        public bool RemovePunctuation
        {
            get => this.removePunctuation;
            set
            {
                this.removePunctuation = value;
                this.NotifyOfPropertyChange(() => RemovePunctuation);
            }
        }

        public BindingList<FileOverwriteBehaviour> FileOverwriteBehaviourList { get; set; }

        public int SelectedOverwriteBehaviour
        {
            get => this.selectedOverwriteBehaviour;
            set
            {
                if (value == this.selectedOverwriteBehaviour) return;
                this.selectedOverwriteBehaviour = value;
                this.NotifyOfPropertyChange(() => this.SelectedOverwriteBehaviour);
            }
        }

        public BindingList<AutonameFileCollisionBehaviour> AutonameFileCollisionBehaviours { get; set; }

        public int SelectedCollisionBehaviour
        {
            get => this.selectedCollisionBehaviour;
            set
            {
                if (value == this.selectedCollisionBehaviour) return;
                this.selectedCollisionBehaviour = value;

                this.ShowPrePostFilenameBox = this.selectedCollisionBehaviour >= 1;

                this.NotifyOfPropertyChange(() => this.SelectedCollisionBehaviour);
            }
        }

        public string PrePostFilenameText
        {
            get => this.prePostFilenameText;
            set
            {
                if (value == this.prePostFilenameText) return;

                if (this.IsValidAutonameFormat(value, false))
                {
                    this.prePostFilenameText = value;
                }

                this.NotifyOfPropertyChange(() => this.PrePostFilenameText);
            }
        }

        public bool ShowPrePostFilenameBox
        {
            get => this.showPrePostFilenameBox;
            set
            {
                if (value == this.showPrePostFilenameBox) return;
                this.showPrePostFilenameBox = value;
                this.NotifyOfPropertyChange(() => this.ShowPrePostFilenameBox);
            }
        }

        public bool AlwaysUseDefaultPath
        {
            get => this.alwaysUseDefaultPath;
            set
            {
                if (value == this.alwaysUseDefaultPath)
                {
                    return;
                }

                this.alwaysUseDefaultPath = value;
                this.NotifyOfPropertyChange(() => this.AlwaysUseDefaultPath);
            }
        }

        public BindingList<PlaceHolderBucket> OutputFilenamePlaceholders
        {
            get
            {
                return new BindingList<PlaceHolderBucket>
                {
                    new PlaceHolderBucket { Name = Constants.Source },
                    new PlaceHolderBucket { Name = Constants.Title },
                    new PlaceHolderBucket { Name = Constants.Chapters},
                    new PlaceHolderBucket { Name = Constants.CreationDate },
                    new PlaceHolderBucket { Name = Constants.CreationTime },
                    new PlaceHolderBucket { Name = Constants.ModificationDate },
                    new PlaceHolderBucket { Name = Constants.ModificationTime },
                    new PlaceHolderBucket { Name = Constants.Date },
                    new PlaceHolderBucket { Name = Constants.Time },
                    new PlaceHolderBucket { Name = Constants.QualityBitrate },
                    new PlaceHolderBucket { Name = Constants.QualityType },
                    new PlaceHolderBucket { Name = Constants.Preset },
                    new PlaceHolderBucket { Name = Constants.EncoderBitDepth },
                    new PlaceHolderBucket { Name = Constants.StorageWidth },
                    new PlaceHolderBucket { Name = Constants.StorageHeight },
                    new PlaceHolderBucket { Name = Constants.Codec },
                    new PlaceHolderBucket { Name = Constants.Encoder },
                };
            }
        }

        public BindingList<PlaceHolderBucket> PathFilenamePlaceholders
        {
            get
            {
                return new BindingList<PlaceHolderBucket>
                       {
                           new PlaceHolderBucket { Name = Constants.SourcePath },
                           new PlaceHolderBucket { Name = Constants.SourceFolderName },
                           new PlaceHolderBucket { Name = Constants.Source }
                       };
            }
        }


        public BindingList<PlaceHolderBucket> WhenDoneArguments
        {
            get
            {
                return new BindingList<PlaceHolderBucket>
                       {
                           new PlaceHolderBucket { Name = Constants.SourceArg },
                           new PlaceHolderBucket { Name = Constants.DestinationArg },
                           new PlaceHolderBucket { Name = Constants.ExitCodeArg }
                       };
            }
        }
        
        public bool UseIsoDateFormat
        {
            get => this.useIsoDateFormat;
            set
            {
                if (value == this.useIsoDateFormat) return;
                this.useIsoDateFormat = value;
                this.OnPropertyChanged();
            }
        }

        /* Preview */

        public string VLCPath
        {
            get => this.vlcPath;

            set
            {
                this.vlcPath = value;
                this.NotifyOfPropertyChange(() => this.VLCPath);
            }
        }


        /* System and Logging */

        public bool CopyLogToEncodeDirectory
        {
            get => this.copyLogToEncodeDirectory;

            set
            {
                this.copyLogToEncodeDirectory = value;
                this.NotifyOfPropertyChange(() => this.CopyLogToEncodeDirectory);
            }
        }

        public bool CopyLogToSpecifiedLocation
        {
            get => this.copyLogToSpecifiedLocation;

            set
            {
                this.copyLogToSpecifiedLocation = value;
                this.NotifyOfPropertyChange(() => this.CopyLogToSpecifiedLocation);
            }
        }

        public bool ClearOldLogs
        {
            get => this.clearOldLogs;

            set
            {
                this.clearOldLogs = value;
                this.NotifyOfPropertyChange(() => this.ClearOldLogs);
            }
        }

        public string LogDirectory
        {
            get => this.logDirectory;

            set
            {
                this.logDirectory = value;
                this.NotifyOfPropertyChange(() => this.LogDirectory);
            }
        }

        public bool PreventSleep
        {
            get => this.preventSleep;

            set
            {
                this.preventSleep = value;
                this.NotifyOfPropertyChange(() => this.PreventSleep);
            }
        }

        public bool PauseOnLowDiskspace
        {
            get => this.pauseOnLowDiskspace;

            set
            {
                this.pauseOnLowDiskspace = value;
                this.NotifyOfPropertyChange(() => this.PauseOnLowDiskspace);
            }
        }

        public long PauseOnLowDiskspaceLevel
        {
            get => this.pauseOnLowDiskspaceLevel;

            set
            {
                this.pauseOnLowDiskspaceLevel = value;
                this.NotifyOfPropertyChange(() => this.pauseOnLowDiskspaceLevel);
            }
        }

        public BindingList<ProcessPriority> PriorityLevelOptions { get; } = new BindingList<ProcessPriority>(EnumHelper<ProcessPriority>.GetEnumList().ToList());

        public string SelectedGranularity
        {
            get => this.selectedGranularity;

            set
            {
                this.selectedGranularity = value;
                this.NotifyOfPropertyChange(() => this.SelectedGranularity);
            }
        }

        public ProcessPriority SelectedPriority
        {
            get => this.selectedPriority;

            set
            {
                this.selectedPriority = value;
                this.NotifyOfPropertyChange(() => this.SelectedPriority);
                this.SetProcessPriority(value);
            }
        }

        private void SetProcessPriority(ProcessPriority value)
        {
            if (this.RemoteServiceEnabled)
            {
                return; // We run as "Normal" in Remote mode.
            }

            // Set the Process Priority
            switch (value)
            {
                case ProcessPriority.High:
                    Process.GetCurrentProcess().PriorityClass = ProcessPriorityClass.High;
                    break;
                case ProcessPriority.AboveNormal:
                    Process.GetCurrentProcess().PriorityClass = ProcessPriorityClass.AboveNormal;
                    break;
                case ProcessPriority.Normal:
                    Process.GetCurrentProcess().PriorityClass = ProcessPriorityClass.Normal;
                    break;
                case ProcessPriority.BelowNormal:
                    Process.GetCurrentProcess().PriorityClass = ProcessPriorityClass.BelowNormal;
                    break;
                case ProcessPriority.Low:
                    Process.GetCurrentProcess().PriorityClass = ProcessPriorityClass.Idle;
                    break;
                default:
                    Process.GetCurrentProcess().PriorityClass = ProcessPriorityClass.BelowNormal;
                    break;
            }
        }

        /* Advanced */

        public BindingList<string> ConstantQualityGranularity
        {
            get => this.constantQualityGranularity;

            set
            {
                this.constantQualityGranularity = value;
                this.NotifyOfPropertyChange(() => this.ConstantQualityGranularity);
            }
        }

        public bool DisableLibdvdNav
        {
            get => this.disableLibdvdNav;

            set
            {
                this.disableLibdvdNav = value;
                this.NotifyOfPropertyChange(() => this.DisableLibdvdNav);
            }
        }

        public BindingList<int> LogVerbosityOptions
        {
            get => this.logVerbosityOptions;

            set
            {
                this.logVerbosityOptions = value;
                this.NotifyOfPropertyChange(() => this.LogVerbosityOptions);
            }
        }

        public long MinLength
        {
            get => this.minLength;

            set
            {
                this.minLength = value;
                this.NotifyOfPropertyChange(() => this.MinLength);
            }
        }

        public bool MinimiseToTray
        {
            get => this.minimiseToTray;

            set
            {
                this.minimiseToTray = value;
                this.NotifyOfPropertyChange(() => this.MinimiseToTray);
            }
        }

        public BindingList<int> PreviewPicturesToScan
        {
            get => this.previewPicturesToScan;

            set
            {
                this.previewPicturesToScan = value;
                this.NotifyOfPropertyChange(() => this.PreviewPicturesToScan);
            }
        }

        public int SelectedPreviewCount
        {
            get => this.selectedPreviewCount;

            set
            {
                this.selectedPreviewCount = value;
                this.NotifyOfPropertyChange(() => this.SelectedPreviewCount);
            }
        }

        public int SelectedVerbosity
        {
            get => this.selectedVerbosity;

            set
            {
                this.selectedVerbosity = value;
                this.NotifyOfPropertyChange(() => this.SelectedVerbosity);
            }
        }

        public bool ClearQueueOnEncodeCompleted
        {
            get => this.clearQueueOnEncodeCompleted;
            set
            {
                this.clearQueueOnEncodeCompleted = value;
                this.NotifyOfPropertyChange(() => this.ClearQueueOnEncodeCompleted);
            }
        }

        public bool PauseOnLowBattery
        {
            get => this.pauseOnLowBattery;
            set
            {
                if (value == this.pauseOnLowBattery) return;
                this.pauseOnLowBattery = value;
                this.NotifyOfPropertyChange(() => this.PauseOnLowBattery);
            }
        }

        public int LowBatteryLevel
        {
            get => this.lowBatteryLevel;
            set
            {
                if (value == this.lowBatteryLevel) return;
                this.lowBatteryLevel = value;
                this.NotifyOfPropertyChange(() => this.LowBatteryLevel);
            }
        }

        public BindingList<string> ExcludedFileExtensions
        {
            get => this.excludedFileExtensions;
            set
            {
                if (Equals(value, this.excludedFileExtensions)) return;
                this.excludedFileExtensions = value;
                this.OnPropertyChanged();
            }
        }

        public string NewExtension { get; set; }

        public SimpleRelayCommand<string> RemoveExtensionCommand { get; set; }

        public bool RecursiveFolderScan
        {
            get => this.recursiveFolderScan;
            set
            {
                if (value == this.recursiveFolderScan) return;
                this.recursiveFolderScan = value;
                this.NotifyOfPropertyChange(() => this.RecursiveFolderScan);
            }
        }

        /* Video */
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
                this.NotifyOfPropertyChange(() => this.CanSetQsvDecForOtherEncodes);
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
                this.NotifyOfPropertyChange(() => this.IsNvdecAvailable);
            }
        }

        public bool EnableQuickSyncDecoding
        {
            get => this.enableQuickSyncDecoding && IsQuickSyncAvailable;

            set
            {
                if (value.Equals(this.enableQuickSyncDecoding))
                {
                    return;
                }
                this.enableQuickSyncDecoding = value;
                this.NotifyOfPropertyChange(() => this.EnableQuickSyncDecoding);
                this.NotifyOfPropertyChange(() => this.IsUseQsvDecAvailable);
                this.NotifyOfPropertyChange(() => this.CanSetQsvDecForOtherEncodes);
            }
        }

        public bool EnableQuickSyncLowPower
        {
            get => this.enableQuickSyncLowPower;
            set
            {
                if (value == this.enableQuickSyncLowPower)
                {
                    return;
                }

                this.enableQuickSyncLowPower = value;
                this.NotifyOfPropertyChange(() => this.EnableQuickSyncLowPower);
            }
        }

        public bool EnableQuickSyncHyperEncode
        {
            get => this.enableQuickSyncHyperEncode;
            set
            {
                if (value == this.enableQuickSyncHyperEncode)
                {
                    return;
                }

                this.enableQuickSyncHyperEncode = value;
                this.NotifyOfPropertyChange(() => this.EnableQuickSyncHyperEncode);
            }
        }

        public VideoScaler SelectedScalingMode { get; set; }

        public bool IsQuickSyncAvailable { get; } = HandBrakeHardwareEncoderHelper.IsQsvAvailable;

        public bool IsQuickSyncHyperEncodeAvailable { get; } = HandBrakeHardwareEncoderHelper.IsQsvHyperEncodeAvailable;

        public bool IsVceAvailable { get; } = HandBrakeHardwareEncoderHelper.IsVceH264Available;

        public bool IsNvencAvailable { get; } = HandBrakeHardwareEncoderHelper.IsNVEncH264Available;

        public bool IsUseQsvDecAvailable => this.IsQuickSyncAvailable && this.EnableQuickSyncDecoding;

        public bool IsNvdecAvailable => HandBrakeHardwareEncoderHelper.IsNVDecAvailable && this.EnableNvencEncoder;

        public bool UseQSVDecodeForNonQSVEnc
        {
            get => this.useQsvDecodeForNonQsvEnc;

            set
            {
                if (value == this.useQsvDecodeForNonQsvEnc) return;
                this.useQsvDecodeForNonQsvEnc = value;
                this.NotifyOfPropertyChange(() => this.UseQSVDecodeForNonQSVEnc);
            }
        }

        public bool CanSetQsvDecForOtherEncodes => this.EnableQuickSyncDecoding && this.IsQuickSyncAvailable && this.EnableQuickSyncEncoding;

        public BindingList<VideoScaler> ScalingOptions { get; } = new BindingList<VideoScaler>(EnumHelper<VideoScaler>.GetEnumList().ToList());

        public bool IsHardwareFallbackMode => HandBrakeUtils.IsInitNoHardware();

        public bool IsSafeMode => HandBrakeHardwareEncoderHelper.IsSafeMode;

        public bool IsHardwareOptionsVisible => !IsSafeMode && !IsHardwareFallbackMode;

        public bool IsAutomaticSafeMode { get; private set; }

        public bool EnableNvDecSupport
        {
            get => this.enableNvDecSupport;
            set
            {
                if (value == this.enableNvDecSupport) return;
                this.enableNvDecSupport = value;
                this.NotifyOfPropertyChange(() => this.EnableNvDecSupport);
            }
        }

        /* About HandBrake */

        public string Version { get; } = string.Format("{0}", HandBrakeVersionHelper.GetVersion());

        public string UpdateMessage
        {
            get => this.updateMessage;
            set
            {
                this.updateMessage = value;
                this.NotifyOfPropertyChange(() => this.UpdateMessage);
            }
        }

        public bool UpdateAvailable
        {
            get => this.updateAvailable;
            set
            {
                this.updateAvailable = value;
                this.NotifyOfPropertyChange(() => this.UpdateAvailable);
            }
        }

        public int DownloadProgressPercentage
        {
            get => this.downloadProgressPercentage;
            set
            {
                this.downloadProgressPercentage = value;
                this.NotifyOfPropertyChange(() => this.DownloadProgressPercentage);
            }
        }

        public bool RemoteServiceEnabled
        {
            get => this.remoteServiceEnabled;
            set
            {
                if (value == this.remoteServiceEnabled)
                {
                    return;
                }

                this.remoteServiceEnabled = value;
                this.NotifyOfPropertyChange(() => this.RemoteServiceEnabled);
            }
        }

        public int RemoteServicePort
        {
            get => this.remoteServicePort;
            set
            {
                if (value == this.remoteServicePort)
                {
                    return;
                }

                if (value > 32767 || value < 5000)
                {
                    this.errorService.ShowMessageBox(
                        Resources.OptionsView_RemotePortLimit,
                        Resources.Error,
                        MessageBoxButton.OK,
                        MessageBoxImage.Error);
                    return; // Allow only valid ports, not in the ephemeral range
                }
                
                this.remoteServicePort = value;
                this.NotifyOfPropertyChange(() => this.RemoteServicePort);
            }
        }

        public bool IsProcessIsolationAllowed { get; } = Portable.IsProcessIsolationEnabled();

        public int SimultaneousEncodes
        {
            get => this.simultaneousEncodes;
            set
            {
                if (value == this.simultaneousEncodes) return;
                this.simultaneousEncodes = value;
                this.NotifyOfPropertyChange(() => this.SimultaneousEncodes);
            }
        }

        public BindingList<int> SimultaneousEncodesList
        {
            get
            {
                BindingList<int> list = new BindingList<int>();
                for (int i = 1; i <= SystemInfo.MaximumSimultaneousInstancesSupported; i++)
                {
                    list.Add(i);
                }

                return list;
            }
        }

        public bool IsSimultaneousEncodesSupported => SystemInfo.MaximumSimultaneousInstancesSupported > 1;
        
        #region Public Methods

        public void Close()
        {
            this.Save();

            IShellViewModel shellViewModel = IoCHelper.Get<IShellViewModel>();
            shellViewModel.DisplayWindow(ShellWindow.MainWindow);
        }

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

        public void BrowseAutoNamePath()
        {
            FolderBrowserDialog dialog = new FolderBrowserDialog { Description = Resources.OptionsView_SelectFolder, UseDescriptionForTitle = true, SelectedPath = this.AutoNameDefaultPath };
            bool? dialogResult = dialog.ShowDialog();
            if (dialogResult.HasValue && dialogResult.Value)
            {
                this.AutoNameDefaultPath = dialog.SelectedPath;
            }
        }

        public void BrowseVlcPath()
        {
            OpenFileDialog dialog = new OpenFileDialog { Filter = "All files (*.exe)|*.exe", FileName = this.VLCPath };
            bool? dialogResult = dialog.ShowDialog();
            if (dialogResult.HasValue && dialogResult.Value)
            {
                this.VLCPath = dialog.FileName;
            }
        }

        public void BrowseLogPath()
        {
            FolderBrowserDialog dialog = new FolderBrowserDialog { Description = Resources.OptionsView_SelectFolder, UseDescriptionForTitle = true, SelectedPath = this.LogDirectory };
            bool? dialogResult = dialog.ShowDialog();
            if (dialogResult.HasValue && dialogResult.Value)
            {
                this.LogDirectory = dialog.SelectedPath;
            }
        }

        public void ViewLogDirectory()
        {
            string logDir = DirectoryUtilities.GetLogDirectory();
            string windir = Environment.GetEnvironmentVariable("WINDIR");
            Process prc = new Process { StartInfo = { FileName = windir + @"\explorer.exe", Arguments = logDir } };
            prc.Start();
        }

        public void ClearLogHistory()
        {
            MessageBoxResult result = this.errorService.ShowMessageBox(Resources.OptionsView_ClearLogDirConfirm, Resources.OptionsView_ClearLogs,
                                                  MessageBoxButton.YesNo, MessageBoxImage.Question);
            if (result == MessageBoxResult.Yes)
            {
                GeneralUtilities.ClearLogFiles(0);
                this.errorService.ShowMessageBox(Resources.OptionsView_LogsCleared, Resources.OptionsView_Notice, MessageBoxButton.OK, MessageBoxImage.Information);
            }
        }

        public void DownloadUpdate()
        {
            this.UpdateMessage = Resources.OptionsView_PreparingUpdate;
            this.updateService.DownloadFile(this.updateInfo.DownloadFile, this.updateInfo.Signature, this.DownloadComplete, this.DownloadProgress);
        }

        public void PerformUpdateCheck()
        {
            this.UpdateMessage = Resources.OptionsView_CheckingForUpdates;
            this.updateService.CheckForUpdates(this.UpdateCheckComplete);
        }

        public void BrowseWhenDoneAudioFile()
        {
            OpenFileDialog dialog = new OpenFileDialog() { Filter = "All Files|*.wav;*.mp3", FileName = this.WhenDoneAudioFileFullPath };
            bool? dialogResult = dialog.ShowDialog();
            if (dialogResult.HasValue && dialogResult.Value)
            {
                this.WhenDoneAudioFile = Path.GetFileNameWithoutExtension(dialog.FileName);
                this.WhenDoneAudioFileFullPath = dialog.FileName;
            }
            else
            {
                this.WhenDoneAudioFile = null;
                this.WhenDoneAudioFileFullPath = null;
            }
        }

        public void PlayWhenDoneFile()
        {
            if (!string.IsNullOrEmpty(this.WhenDoneAudioFileFullPath) && File.Exists(this.WhenDoneAudioFileFullPath))
            {
                try
                {
                    var player = new SoundPlayer(this.WhenDoneAudioFileFullPath);
                    player.Play();
                }
                catch (Exception exc)
                {
                    this.logService.LogMessage(string.Format("{1} # {0}{1}", exc, Environment.NewLine));
                }
            }
            else
            {
                this.errorService.ShowMessageBox(
                    Resources.OptionsView_MediaFileNotSet,
                    Resources.Error,
                    MessageBoxButton.OK,
                    MessageBoxImage.Error);
            }
        }

        public void ResetAutomaticSafeMode()
        {
            userSettingService.SetUserSetting(UserSettingConstants.ForceDisableHardwareSupport, false);
            this.errorService.ShowMessageBox(
                Resources.OptionsView_ResetSafeModeMessage,
                Resources.Notice,
                MessageBoxButton.OK,
                MessageBoxImage.Information);

            this.IsAutomaticSafeMode = false;
            this.NotifyOfPropertyChange(() => this.IsAutomaticSafeMode);
        }

        public void ResetAutoNameFormat()
        {
            this.AutonameFormat = "{source}-{title}";
        }

        public void AddExcludedExtension()
        {
            if (!string.IsNullOrEmpty(NewExtension))
            {
                NewExtension = NewExtension.Replace(".", string.Empty);

                if (this.ExcludedFileExtensions.Contains(NewExtension, StringComparer.OrdinalIgnoreCase))
                {
                    this.errorService.ShowMessageBox(Resources.Options_ExtensionExists, Resources.Error, MessageBoxButton.OK, MessageBoxImage.Error);
                    return;
                }

                this.ExcludedFileExtensions.Add(this.NewExtension);
                this.NewExtension = null;
                this.NotifyOfPropertyChange(() => this.NewExtension);
            }
        }

        public void RemoveExcludedExtension(string extension)
        {
            if (!string.IsNullOrEmpty(extension))
            {
                if (this.ExcludedFileExtensions.Contains(extension, StringComparer.OrdinalIgnoreCase))
                {
                    this.ExcludedFileExtensions.Remove(extension);
                }
            }
        }

        #endregion

        public override void OnLoad()
        {
            // #############################
            // General
            // #############################
            string culture = this.userSettingService.GetUserSetting<string>(UserSettingConstants.UiLanguage);
            this.SelectedLanguage = InterfaceLanguageUtilities.FindInterfaceLanguage(culture);
            this.SelectedRightToLeftMode = (RightToLeftMode)this.userSettingService.GetUserSetting<int>(UserSettingConstants.RightToLeftUi);

            this.CheckForUpdatesAllowed = true;
            this.CheckForUpdates = this.userSettingService.GetUserSetting<bool>(UserSettingConstants.UpdateStatus);
            this.CheckForUpdatesFrequency = (UpdateCheck)this.userSettingService.GetUserSetting<int>(UserSettingConstants.DaysBetweenUpdateCheck);
            if (Portable.IsPortable() && !Portable.IsUpdateCheckEnabled())
            {
                this.CheckForUpdates = false;
                this.CheckForUpdatesAllowed = false;
            }

            this.ShowStatusInTitleBar = this.userSettingService.GetUserSetting<bool>(UserSettingConstants.ShowStatusInTitleBar);
            this.ShowPreviewOnSummaryTab = this.userSettingService.GetUserSetting<bool>(UserSettingConstants.ShowPreviewOnSummaryTab);
            this.ShowAddAllToQueue = this.userSettingService.GetUserSetting<bool>(UserSettingConstants.ShowAddAllToQueue);
            this.ShowAddSelectionToQueue = this.userSettingService.GetUserSetting<bool>(UserSettingConstants.ShowAddSelectionToQueue);
            this.DarkThemeMode = (DarkThemeMode)this.userSettingService.GetUserSetting<int>(UserSettingConstants.DarkThemeMode);
            this.SelectedPresetDisplayMode = (PresetDisplayMode)this.userSettingService.GetUserSetting<int>(UserSettingConstants.PresetMenuDisplayMode);

            // #############################
            // When Done
            // #############################

            this.WhenDone = (WhenDone)this.userSettingService.GetUserSetting<int>(UserSettingConstants.WhenCompleteAction);
            if (this.userSettingService.GetUserSetting<bool>(UserSettingConstants.ResetWhenDoneAction))
            {
                this.WhenDone = WhenDone.DoNothing;
            }

            this.SendFileAfterEncode = this.userSettingService.GetUserSetting<bool>(UserSettingConstants.SendFile);
            this.SendFileTo = Path.GetFileNameWithoutExtension(this.userSettingService.GetUserSetting<string>(UserSettingConstants.SendFileTo)) ?? string.Empty;
            this.SendFileToPath = this.userSettingService.GetUserSetting<string>(UserSettingConstants.SendFileTo) ?? string.Empty;
            this.Arguments = this.userSettingService.GetUserSetting<string>(UserSettingConstants.SendFileToArgs) ?? string.Empty;
            this.ResetWhenDoneAction = this.userSettingService.GetUserSetting<bool>(UserSettingConstants.ResetWhenDoneAction);
            this.WhenDonePerformActionImmediately = this.userSettingService.GetUserSetting<bool>(UserSettingConstants.WhenDonePerformActionImmediately);
            this.WhenDoneAudioFile = Path.GetFileNameWithoutExtension(this.userSettingService.GetUserSetting<string>(UserSettingConstants.WhenDoneAudioFile)) ?? string.Empty;
            this.WhenDoneAudioFileFullPath = this.userSettingService.GetUserSetting<string>(UserSettingConstants.WhenDoneAudioFile);
            this.PlaySoundWhenDone = this.userSettingService.GetUserSetting<bool>(UserSettingConstants.PlaySoundWhenDone);
            this.PlaySoundWhenQueueDone = this.userSettingService.GetUserSetting<bool>(UserSettingConstants.PlaySoundWhenQueueDone);
            this.SendSystemNotificationOnEncodeDone = this.userSettingService.GetUserSetting<bool>(UserSettingConstants.NotifyOnEncodeDone);
            this.SendSystemNotificationOnQueueDone = this.userSettingService.GetUserSetting<bool>(UserSettingConstants.NotifyOnQueueDone);

            // #############################
            // Output Settings
            // #############################

            // Enable auto naming feature.)
            this.AutomaticallyNameFiles = this.userSettingService.GetUserSetting<bool>(UserSettingConstants.AutoNaming);

            // Store the auto name path
            this.AutoNameDefaultPath = this.userSettingService.GetUserSetting<string>(UserSettingConstants.AutoNamePath) ?? string.Empty;
            if (string.IsNullOrEmpty(this.autoNameDefaultPath))
                this.AutoNameDefaultPath = Resources.OptionsView_SetDefaultLocationOutputFIle;

            // Store auto name format
            string anf = this.userSettingService.GetUserSetting<string>(UserSettingConstants.AutoNameFormat) ?? string.Empty;
            this.AutonameFormat = this.IsValidAutonameFormat(anf, true) ? anf : "{source}-{title}";

            // Use iPod/iTunes friendly .m4v extension for MP4 files.
            this.SelectedMp4Extension = (Mp4Behaviour)this.userSettingService.GetUserSetting<int>(UserSettingConstants.UseM4v);

            // Remove Underscores
            this.RemoveUnderscores = this.userSettingService.GetUserSetting<bool>(UserSettingConstants.AutoNameRemoveUnderscore);

            // Title case
            this.ChangeToTitleCase = this.userSettingService.GetUserSetting<bool>(UserSettingConstants.AutoNameTitleCase);
            this.RemovePunctuation = this.userSettingService.GetUserSetting<bool>(UserSettingConstants.RemovePunctuation);

            // File Overwrite
            this.FileOverwriteBehaviourList = new BindingList<FileOverwriteBehaviour>();
            this.FileOverwriteBehaviourList.Add(FileOverwriteBehaviour.Ask);
            this.FileOverwriteBehaviourList.Add(FileOverwriteBehaviour.ForceOverwrite);
            this.SelectedOverwriteBehaviour = this.userSettingService.GetUserSetting<int>(UserSettingConstants.FileOverwriteBehaviour);

            // Collision behaviour
            this.AutonameFileCollisionBehaviours = new BindingList<AutonameFileCollisionBehaviour>() { AutonameFileCollisionBehaviour.AppendNumber, AutonameFileCollisionBehaviour.Prefix, AutonameFileCollisionBehaviour.Postfix };
            this.SelectedCollisionBehaviour = this.userSettingService.GetUserSetting<int>(UserSettingConstants.AutonameFileCollisionBehaviour);
            this.PrePostFilenameText = this.userSettingService.GetUserSetting<string>(UserSettingConstants.AutonameFilePrePostString);

            this.AlwaysUseDefaultPath = this.userSettingService.GetUserSetting<bool>(UserSettingConstants.AlwaysUseDefaultPath);

            this.UseIsoDateFormat = this.userSettingService.GetUserSetting<bool>(UserSettingConstants.UseIsoDateFormat);

            // #############################
            // Picture Tab
            // #############################

            // VLC Path
            this.VLCPath = this.userSettingService.GetUserSetting<string>(UserSettingConstants.MediaPlayerPath) ?? string.Empty;

            // #############################
            // Video
            // #############################
            this.EnableQuickSyncDecoding = this.userSettingService.GetUserSetting<bool>(UserSettingConstants.EnableQuickSyncDecoding);
            this.SelectedScalingMode = this.userSettingService.GetUserSetting<VideoScaler>(UserSettingConstants.ScalingMode);
            this.UseQSVDecodeForNonQSVEnc = this.userSettingService.GetUserSetting<bool>(UserSettingConstants.UseQSVDecodeForNonQSVEnc);
            this.EnableQuickSyncLowPower = this.userSettingService.GetUserSetting<bool>(UserSettingConstants.EnableQuickSyncLowPower);
            this.EnableQuickSyncHyperEncode = this.userSettingService.GetUserSetting<bool>(UserSettingConstants.EnableQuickSyncHyperEncode);

            this.EnableQuickSyncEncoding = this.userSettingService.GetUserSetting<bool>(UserSettingConstants.EnableQuickSyncEncoding);
            this.EnableVceEncoder = this.userSettingService.GetUserSetting<bool>(UserSettingConstants.EnableVceEncoder);
            this.EnableNvencEncoder = this.userSettingService.GetUserSetting<bool>(UserSettingConstants.EnableNvencEncoder);
            this.EnableNvDecSupport = this.userSettingService.GetUserSetting<bool>(UserSettingConstants.EnableNvDecSupport);

            // #############################
            // Process
            // #############################

            this.RemoteServiceEnabled = this.userSettingService.GetUserSetting<bool>(UserSettingConstants.ProcessIsolationEnabled);
            this.RemoteServicePort = userSettingService.GetUserSetting<int>(UserSettingConstants.ProcessIsolationPort);
            this.SimultaneousEncodes = userSettingService.GetUserSetting<int>(UserSettingConstants.SimultaneousEncodes);
            if (this.SimultaneousEncodes > 8)
            {
                this.SimultaneousEncodes = 8;
            }

            this.SelectedPriority = (ProcessPriority)userSettingService.GetUserSetting<int>(UserSettingConstants.ProcessPriorityInt);

            this.PreventSleep = userSettingService.GetUserSetting<bool>(UserSettingConstants.PreventSleep);
            this.PauseOnLowDiskspace = userSettingService.GetUserSetting<bool>(UserSettingConstants.PauseOnLowDiskspace);
            this.PauseOnLowDiskspaceLevel = this.userSettingService.GetUserSetting<long>(UserSettingConstants.PauseQueueOnLowDiskspaceLevel);

            // Log Verbosity Level
            this.logVerbosityOptions.Clear();
            this.logVerbosityOptions.Add(0);
            this.logVerbosityOptions.Add(1);
            this.logVerbosityOptions.Add(2);
            this.SelectedVerbosity = userSettingService.GetUserSetting<int>(UserSettingConstants.Verbosity);

            // Logs
            this.CopyLogToEncodeDirectory = userSettingService.GetUserSetting<bool>(UserSettingConstants.SaveLogWithVideo);
            this.CopyLogToSpecifiedLocation = userSettingService.GetUserSetting<bool>(UserSettingConstants.SaveLogToCopyDirectory);

            // The saved log path
            this.LogDirectory = userSettingService.GetUserSetting<string>(UserSettingConstants.SaveLogCopyDirectory) ?? string.Empty;

            this.ClearOldLogs = this.userSettingService.GetUserSetting<bool>(UserSettingConstants.ClearOldLogs);

            // #############################
            // Advanced
            // #############################

            // Minimise to Tray
            this.MinimiseToTray = this.userSettingService.GetUserSetting<bool>(UserSettingConstants.MainWindowMinimize);
            this.ClearQueueOnEncodeCompleted = userSettingService.GetUserSetting<bool>(UserSettingConstants.ClearCompletedFromQueue);

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
            this.SelectedGranularity = userSettingService.GetUserSetting<double>(UserSettingConstants.X264Step).ToString("0.00", CultureInfo.InvariantCulture);

            // Min Title Length
            this.MinLength = this.userSettingService.GetUserSetting<int>(UserSettingConstants.MinScanDuration);

            // Use dvdnav
            this.DisableLibdvdNav = userSettingService.GetUserSetting<bool>(UserSettingConstants.DisableLibDvdNav);

            this.PauseOnLowBattery = userSettingService.GetUserSetting<bool>(UserSettingConstants.PauseEncodingOnLowBattery);
            this.LowBatteryLevel = userSettingService.GetUserSetting<int>(UserSettingConstants.LowBatteryLevel);

            this.ExcludedFileExtensions = new BindingList<string>(userSettingService.GetUserSetting<List<string>>(UserSettingConstants.ExcludedExtensions));
            this.RecursiveFolderScan = userSettingService.GetUserSetting<bool>(UserSettingConstants.RecursiveFolderScan);

            // #############################
            // Safe Mode
            // #############################
            this.IsAutomaticSafeMode = userSettingService.GetUserSetting<bool>(UserSettingConstants.ForceDisableHardwareSupport);
            this.NotifyOfPropertyChange(() => this.IsAutomaticSafeMode);
        }

        public void UpdateSettings()
        {
            this.WhenDone = (WhenDone)this.userSettingService.GetUserSetting<int>(UserSettingConstants.WhenCompleteAction);
            this.ShowAddAllToQueue = this.userSettingService.GetUserSetting<bool>(UserSettingConstants.ShowAddAllToQueue);
            this.ShowAddSelectionToQueue = this.userSettingService.GetUserSetting<bool>(UserSettingConstants.ShowAddSelectionToQueue);
        }

        public void GotoTab(OptionsTab tab)
        {
            this.SelectedTab = tab;
        }

        public void ResetHandBrake()
        {
            MessageBoxResult result = this.errorService.ShowMessageBox(
                Resources.OptionsViewModel_ResetHandBrakeQuestion,
                Resources.OptionsViewModel_ResetHandBrake,
                MessageBoxButton.YesNo,
                MessageBoxImage.Question);

            if (result == MessageBoxResult.Yes)
            {
                this.userSettingService.ResetSettingsToDefaults();
                this.OnLoad();
            }
        }

        public void ResetBuiltInPresets()
        {
            MessageBoxResult result = this.errorService.ShowMessageBox(
                Resources.OptionsViewModel_ResetHandBrakePresetsQuestion,
                Resources.OptionsViewModel_ResetHandBrake,
                MessageBoxButton.YesNo,
                MessageBoxImage.Question);

            if (result == MessageBoxResult.Yes)
            {
                this.presetService.UpdateBuiltInPresets();
                this.errorService.ShowMessageBox(Resources.Presets_ResetComplete, Resources.Presets_ResetHeader, MessageBoxButton.OK, MessageBoxImage.Information);
            }
        }

        public void UninstallNotifications()
        {
            this.SendSystemNotificationOnQueueDone = false;
            this.SendSystemNotificationOnEncodeDone = false;

            this.NotifyOfPropertyChange(() => SendSystemNotificationOnQueueDone);
            this.NotifyOfPropertyChange(() => SendSystemNotificationOnEncodeDone);

            this.notificationService.Uninstall();

            this.errorService.ShowMessageBox(Resources.OptionsView_UninstallMessageBoxText, Resources.OptionsView_UninstallMessageBoxHeader, MessageBoxButton.OK, MessageBoxImage.Information);
        }

        private void Save()
        {
            /* General */
            this.userSettingService.SetUserSetting(UserSettingConstants.UpdateStatus, this.CheckForUpdates);
            this.userSettingService.SetUserSetting(UserSettingConstants.DaysBetweenUpdateCheck, this.CheckForUpdatesFrequency);
            this.userSettingService.SetUserSetting(UserSettingConstants.SendFileTo, this.SendFileToPath);
            this.userSettingService.SetUserSetting(UserSettingConstants.SendFile, this.SendFileAfterEncode);
            this.userSettingService.SetUserSetting(UserSettingConstants.SendFileToArgs, this.Arguments);
            this.userSettingService.SetUserSetting(UserSettingConstants.ShowStatusInTitleBar, this.ShowStatusInTitleBar);
            this.userSettingService.SetUserSetting(UserSettingConstants.ShowPreviewOnSummaryTab, this.ShowPreviewOnSummaryTab);
            this.userSettingService.SetUserSetting(UserSettingConstants.DarkThemeMode, this.DarkThemeMode);
            this.userSettingService.SetUserSetting(UserSettingConstants.UiLanguage, this.SelectedLanguage?.Culture);
            this.userSettingService.SetUserSetting(UserSettingConstants.RightToLeftUi, this.SelectedRightToLeftMode);

            this.userSettingService.SetUserSetting(UserSettingConstants.ShowAddAllToQueue, this.ShowAddAllToQueue);
            this.userSettingService.SetUserSetting(UserSettingConstants.ShowAddSelectionToQueue, this.ShowAddSelectionToQueue);
            this.userSettingService.SetUserSetting(UserSettingConstants.PresetMenuDisplayMode, this.SelectedPresetDisplayMode);

            /* When Done */
            this.userSettingService.SetUserSetting(UserSettingConstants.WhenCompleteAction, (int)this.WhenDone);
            this.userSettingService.SetUserSetting(UserSettingConstants.ResetWhenDoneAction, this.ResetWhenDoneAction);
            this.userSettingService.SetUserSetting(UserSettingConstants.WhenDonePerformActionImmediately, this.WhenDonePerformActionImmediately);
            this.userSettingService.SetUserSetting(UserSettingConstants.PlaySoundWhenDone, this.PlaySoundWhenDone);
            this.userSettingService.SetUserSetting(UserSettingConstants.PlaySoundWhenQueueDone, this.PlaySoundWhenQueueDone);
            this.userSettingService.SetUserSetting(UserSettingConstants.WhenDoneAudioFile, this.WhenDoneAudioFileFullPath);
            this.userSettingService.SetUserSetting(UserSettingConstants.NotifyOnEncodeDone, this.SendSystemNotificationOnEncodeDone);
            this.userSettingService.SetUserSetting(UserSettingConstants.NotifyOnQueueDone, this.SendSystemNotificationOnQueueDone);

            /* Output Files */
            this.userSettingService.SetUserSetting(UserSettingConstants.AutoNaming, this.AutomaticallyNameFiles);
            this.userSettingService.SetUserSetting(UserSettingConstants.AutoNameFormat, this.AutonameFormat);
            this.userSettingService.SetUserSetting(UserSettingConstants.AutoNamePath, this.AutoNameDefaultPath);
            this.userSettingService.SetUserSetting(UserSettingConstants.UseM4v, (int)this.SelectedMp4Extension);
            this.userSettingService.SetUserSetting(UserSettingConstants.AutoNameRemoveUnderscore, this.RemoveUnderscores);
            this.userSettingService.SetUserSetting(UserSettingConstants.AutoNameTitleCase, this.ChangeToTitleCase);
            this.userSettingService.SetUserSetting(UserSettingConstants.RemovePunctuation, this.RemovePunctuation);
            this.userSettingService.SetUserSetting(UserSettingConstants.FileOverwriteBehaviour, this.SelectedOverwriteBehaviour);
            this.userSettingService.SetUserSetting(UserSettingConstants.AutonameFileCollisionBehaviour, this.SelectedCollisionBehaviour);
            this.userSettingService.SetUserSetting(UserSettingConstants.AutonameFilePrePostString, this.PrePostFilenameText);
            this.userSettingService.SetUserSetting(UserSettingConstants.AlwaysUseDefaultPath, this.AlwaysUseDefaultPath);
            this.userSettingService.SetUserSetting(UserSettingConstants.UseIsoDateFormat, this.UseIsoDateFormat);

            /* Previews */
            this.userSettingService.SetUserSetting(UserSettingConstants.MediaPlayerPath, this.VLCPath);

            /* Video */
            this.userSettingService.SetUserSetting(UserSettingConstants.EnableQuickSyncDecoding, this.EnableQuickSyncDecoding);
            this.userSettingService.SetUserSetting(UserSettingConstants.ScalingMode, this.SelectedScalingMode);
            this.userSettingService.SetUserSetting(UserSettingConstants.UseQSVDecodeForNonQSVEnc, this.UseQSVDecodeForNonQSVEnc);
            this.userSettingService.SetUserSetting(UserSettingConstants.EnableQuickSyncHyperEncode, this.EnableQuickSyncHyperEncode);
            this.userSettingService.SetUserSetting(UserSettingConstants.EnableQuickSyncEncoding, this.EnableQuickSyncEncoding);
            this.userSettingService.SetUserSetting(UserSettingConstants.EnableVceEncoder, this.EnableVceEncoder);
            this.userSettingService.SetUserSetting(UserSettingConstants.EnableNvencEncoder, this.EnableNvencEncoder);
            this.userSettingService.SetUserSetting(UserSettingConstants.EnableNvDecSupport, this.EnableNvDecSupport);
            this.userSettingService.SetUserSetting(UserSettingConstants.EnableQuickSyncLowPower, this.EnableQuickSyncLowPower);

            /* System and Logging */
            this.userSettingService.SetUserSetting(UserSettingConstants.ProcessPriorityInt, (int)this.SelectedPriority);
            this.userSettingService.SetUserSetting(UserSettingConstants.PreventSleep, this.PreventSleep);
            this.userSettingService.SetUserSetting(UserSettingConstants.PauseOnLowDiskspace, this.PauseOnLowDiskspace);
            this.userSettingService.SetUserSetting(UserSettingConstants.PauseQueueOnLowDiskspaceLevel, this.PauseOnLowDiskspaceLevel);
            this.userSettingService.SetUserSetting(UserSettingConstants.Verbosity, this.SelectedVerbosity);
            this.userSettingService.SetUserSetting(UserSettingConstants.SaveLogWithVideo, this.CopyLogToEncodeDirectory);
            this.userSettingService.SetUserSetting(UserSettingConstants.SaveLogToCopyDirectory, this.CopyLogToSpecifiedLocation);
            this.userSettingService.SetUserSetting(UserSettingConstants.SaveLogCopyDirectory, this.LogDirectory);
            this.userSettingService.SetUserSetting(UserSettingConstants.ClearOldLogs, this.ClearOldLogs);

            /* Advanced */
            this.userSettingService.SetUserSetting(UserSettingConstants.MainWindowMinimize, this.MinimiseToTray);
            this.userSettingService.SetUserSetting(UserSettingConstants.ClearCompletedFromQueue, this.ClearQueueOnEncodeCompleted);
            this.userSettingService.SetUserSetting(UserSettingConstants.PreviewScanCount, this.SelectedPreviewCount);
            this.userSettingService.SetUserSetting(UserSettingConstants.X264Step, double.Parse(this.SelectedGranularity, CultureInfo.InvariantCulture));
            this.userSettingService.SetUserSetting(UserSettingConstants.ExcludedExtensions, new List<string>(this.ExcludedFileExtensions));
            this.userSettingService.SetUserSetting(UserSettingConstants.RecursiveFolderScan, this.RecursiveFolderScan);

            int value;
            if (int.TryParse(this.MinLength.ToString(CultureInfo.InvariantCulture), out value))
            {
                this.userSettingService.SetUserSetting(UserSettingConstants.MinScanDuration, value);
            }

            this.userSettingService.SetUserSetting(UserSettingConstants.DisableLibDvdNav, this.DisableLibdvdNav);

            this.userSettingService.SetUserSetting(UserSettingConstants.PauseEncodingOnLowBattery, this.PauseOnLowBattery);
            this.userSettingService.SetUserSetting(UserSettingConstants.LowBatteryLevel, this.LowBatteryLevel);

            this.userSettingService.SetUserSetting(UserSettingConstants.ProcessIsolationEnabled, this.RemoteServiceEnabled);
            this.userSettingService.SetUserSetting(UserSettingConstants.ProcessIsolationPort, this.RemoteServicePort);
            this.userSettingService.SetUserSetting(UserSettingConstants.SimultaneousEncodes, this.SimultaneousEncodes);
        }

        public void LaunchHelp()
        {
            Process.Start("explorer.exe", "https://handbrake.fr/docs/en/latest/technical/preferences.html");
        }

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
                "{0} {1}% - {2}k of {3}k", Resources.OptionsView_Downloading, this.DownloadProgressPercentage, (info.BytesRead / 1024), (info.TotalBytes / 1024));
        }

        private void DownloadComplete(DownloadStatus info)
        {
            this.UpdateAvailable = false;
            this.UpdateMessage = info.WasSuccessful ? Resources.OptionsViewModel_UpdateDownloaded : info.Message;

            if (info.WasSuccessful)
            {
                try
                {
                    // Elevation required to run the installer.
                    Process installer = new Process();
                    installer = new Process
                                {
                                    StartInfo =
                                    {
                                        FileName = Path.Combine(Path.GetTempPath(), "handbrake-setup.exe"),
                                        UseShellExecute = true,
                                        Verb = "runas"
                                    }
                                };

                    installer.Start();
                    ThreadHelper.OnUIThread(() => Application.Current.Shutdown());
                }
                catch (Exception exc)
                {
                    Console.WriteLine(exc);
                }
            }
        }

        private bool IsValidAutonameFormat(string input, bool isSilent)
        {
            if (string.IsNullOrEmpty(input))
            {
                return true;
            }

            char[] invalidchars = Path.GetInvalidFileNameChars();
            Array.Sort(invalidchars);
            foreach (var characterToTest in input)
            {
                if (Array.BinarySearch(invalidchars, characterToTest) >= 0)
                {
                    if (!isSilent)
                    {
                        this.errorService.ShowMessageBox(
                            Resources.OptionsView_InvalidFileFormatChars,
                            Resources.Error,
                            MessageBoxButton.OK,
                            MessageBoxImage.Error);
                    }
                    return false;
                }
            }

            return true;
        }

        private bool IsAutonamePathValid(string path)
        {
            if (string.IsNullOrEmpty(path))
            {
                return true;
            }

            if (path.Contains(Constants.SourcePath) && path.Contains(Constants.SourceFolderName))
            {
                int indexOfSourcePath = path.IndexOf(Constants.SourcePath, StringComparison.Ordinal);
                int indexOfSourceFolderName = path.IndexOf(Constants.SourceFolderName, StringComparison.Ordinal);

                if (indexOfSourceFolderName < indexOfSourcePath)
                {
                    this.errorService.ShowMessageBox(
                        string.Format(Resources.OptionsViewModel_InvalidAutonamePath, Constants.SourceFolderName, Constants.SourcePath),
                        Resources.Error,
                        MessageBoxButton.OK,
                        MessageBoxImage.Error);
                    return false;
                }
            }
            
            return true;
        }
    }
}