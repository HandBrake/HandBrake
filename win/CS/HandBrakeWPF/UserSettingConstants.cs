// --------------------------------------------------------------------------------------------------------------------
// <copyright file="UserSettingConstants.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Constants for the User Settings Service
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF
{
    using System;

    /// <summary>
    /// Constants for the User Settings Service
    /// </summary>
    public class UserSettingConstants
    {
        public const string AutoNameFormat = "autoNameFormat";
        public const string AutoNamePath = "autoNamePath";
        public const string AutoNameRemoveUnderscore = "AutoNameRemoveUnderscore";
        public const string AutoNameTitleCase = "AutoNameTitleCase";
        public const string AutoNaming = "autoNaming";
        public const string ClearOldLogs = "clearOldLogs";
        public const string DaysBetweenUpdateCheck = "daysBetweenUpdateCheck";
        public const string UseExternalPlayer = "previewUseExternalPlayer";
        public const string LastUpdateCheckDate = "lastUpdateCheckDate";
        public const string MainWindowMinimize = "MainWindowMinimize";
        public const string UpdateStatus = "updateStatus";
        public const string UseM4v = "m4vExtensionMode";
        public const string MediaPlayerPath = "VLC_Path";
        public const string InstanceId = "InstanceId";
        public const string X264Step = "X264Step";
        public const string LastPreviewDuration = "LastPreviewDuration";
        public const string WhenCompleteAction = "WhenCompletePerformAction";
        public const string SendFile = "SendFile";
        public const string SendFileTo = "SendFileTo";
        public const string SendFileToArgs = "SendFileToArgs";
        public const string PreventSleep = "PreventSleep";
        public const string PauseOnLowDiskspace = "PauseOnLowDiskspace";
        public const string PauseQueueOnLowDiskspaceLevel = "LowDiskSpaceWarningLevelInBytes";
        public const string RemovePunctuation = "RemovePunctuation";
        public const string ResetWhenDoneAction = "ResetWhenDoneAction";
        public const string DisableLibDvdNav = "DisableLibDvdNav";
        public const string EnableQuickSyncDecoding = "EnableQuickSyncDecoding";
        public const string UseQSVDecodeForNonQSVEnc = "UseQSVDecodeForNonQSVEnc";
        public const string ScalingMode = "ScalingMode";
        public const string PreviewScanCount = "previewScanCount";
        public const string Verbosity = "Verbosity";
        public const string MinScanDuration = "MinTitleScanDuration";
        public const string KeepDuplicateTitles = "KeepDuplicateTitles";
        public const string ProcessPriorityInt = "ProcessPriorityInt";
        public const string SaveLogToCopyDirectory = "SaveLogToCopyDirectory";
        public const string SaveLogWithVideo = "SaveLogWithVideo";
        public const string SaveLogCopyDirectory = "SaveLogCopyDirectory";
        public const string ClearCompletedFromQueue = "ClearCompletedFromQueue";
        public const string ForcePresetReset = "ForcePresetReset";
        public const string PresetExpandedStateList = "PresetExpandedStateList";
        public const string ShowStatusInTitleBar = "ShowStatusInTitleBar";
        public const string ShowPreviewOnSummaryTab = "ShowPreviewOnSummaryTab";
        public const string PlaySoundWhenDone = "PlaySoundWhenDone";
        public const string PlaySoundWhenQueueDone = "PlaySoundWhenQueueDone";
        public const string WhenDoneAudioFile = "WhenDoneAudioFile";
        public const string ProcessIsolationEnabled = "ProcessIsolationEnabled";
        public const string ProcessIsolationPort = "ProcessIsolationPort";
        public const string EnableQuickSyncHyperEncode = "EnableQuickSyncHyperEncode";
        public const string EnableDirectXDecoding = "EnableDirectXDecoding";
        public const string UiLanguage = "UiLanguage";
        public const string ShowAddAllToQueue = "ShowAddAllToQueue";
        public const string ShowAddSelectionToQueue = "ShowAddSelectionToQueue";
        public const string FileOverwriteBehaviour = "FileOverwriteBehaviour";
        public const string AutonameFileCollisionBehaviour = "AutonameFileCollisionBehaviour";
        public const string AutonameFilePrePostString = "AutonameFilePrePostString";
        public const string WhenDonePerformActionImmediately = "WhenDonePerformActionImmediately";
        public const string DarkThemeMode = "DarkThemeMode";
        public const string AlwaysUseDefaultPath = "AlwaysUseDefaultPath";
        public const string PauseEncodingOnLowBattery = "PauseEncodingOnLowBattery";
        public const string LowBatteryLevel = "LowBatteryLevel";
        public const string EnableQuickSyncLowPower = "EnableQuickSyncLowPower";
        public const string SimultaneousEncodes = "SimultaneousEncodes";
        public const string PreviewShowPictureSettingsOverlay = "PreviewShowPictureSettingsOverlay";
        public const string RunCounter = "HandBrakeRunCounter";
        public const string ForceSoftwareRendering = "ForceSoftwareRendering";
        public const string PresetMenuDisplayMode = "PresetMenuDisplayMode";
        public const string NotifyOnEncodeDone = "NotifyOnEncodeDone";
        public const string NotifyOnQueueDone = "NotifyOnQueueDone";
        public const string RightToLeftUi = "RightToLeftUi";
        public const string ForceDisableHardwareSupport = "ForceDisableHardwareSupport";
        public const string IsUpdateAvailableBuild = "IsUpdateAvailableBuild";
        public const string EnableNvDecSupport = "EnableNvDecSupport";
        public const string UseIsoDateFormat = "UseIsoDateFormat";
        public const string ExtendedQueueDisplay = "ExtendedQueueDisplay";
        public const string HardwareDetectTimeoutSeconds = "HardwareDetectTimeoutSeconds";
        public const string ShowPresetDesc = "ShowPresetDescription";
        public const string ExcludedExtensions = "ExcludedFileExtensions";
        public static string RecursiveFolderScan = "RecursiveFolderScan";
        public static string SimpleQueueView = "SimpleQueueView";
        public static string IsLegacyMenuShown = "IsLegacyMenuShown";
    }
}