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
    /// <summary>
    /// Constants for the User Settings Service
    /// </summary>
    public class UserSettingConstants
    {
        #region Constants and Fields

        /// <summary>
        /// Auto name format
        /// </summary>
        public const string AutoNameFormat = "autoNameFormat";

        /// <summary>
        /// Autoname path
        /// </summary>
        public const string AutoNamePath = "autoNamePath";

        /// <summary>
        /// Auto Name Remove underscore
        /// </summary>
        public const string AutoNameRemoveUnderscore = "AutoNameRemoveUnderscore";

        /// <summary>
        /// Auto Name Title Case
        /// </summary>
        public const string AutoNameTitleCase = "AutoNameTitleCase";

        /// <summary>
        /// Auto Naming
        /// </summary>
        public const string AutoNaming = "autoNaming";

        /// <summary>
        /// Clear old logs
        /// </summary>
        public const string ClearOldLogs = "clearOldLogs";

        /// <summary>
        /// Update check interval
        /// </summary>
        public const string DaysBetweenUpdateCheck = "daysBetweenUpdateCheck";

        /// <summary>
        /// Use Default Player
        /// </summary>
        public const string DefaultPlayer = "defaultPlayer";

        /// <summary>
        /// Last Update Check
        /// </summary>
        public const string LastUpdateCheckDate = "lastUpdateCheckDate";

        /// <summary>
        /// Main Window Minimise
        /// </summary>
        public const string MainWindowMinimize = "MainWindowMinimize";

        /// <summary>
        /// Min Title Length
        /// </summary>
        public const string MinTitleLength = "MinTitleLength";

        /// <summary>
        /// Update Status
        /// </summary>
        public const string UpdateStatus = "updateStatus";

        /// <summary>
        /// Use m4v
        /// </summary>
        public const string UseM4v = "useM4v";

        /// <summary>
        /// Vlc Path
        /// </summary>
        public const string VLCPath = "VLC_Path";

        /// <summary>
        /// The Instance Id
        /// </summary>
        public const string InstanceId = "InstanceId";

        /// <summary>
        /// The X264 Stepper 
        /// </summary>
        public const string X264Step = "X264Step";

        /// <summary>
        /// The show advanced tab.
        /// </summary>
        public const string ShowAdvancedTab = "ShowAdvancedTab";

        /// <summary>
        /// The last preview duration
        /// </summary>
        public const string LastPreviewDuration = "LastPreviewDuration";

        /// <summary>
        /// When Complete Action
        /// </summary>
        public const string WhenCompleteAction = "WhenCompleteAction";

        /// <summary>
        /// Send file enabled.
        /// </summary>
        public const string SendFile = "SendFile";

        /// <summary>
        /// Send file to application path
        /// </summary>
        public const string SendFileTo = "SendFileTo";

        /// <summary>
        /// Send file to arguments
        /// </summary>
        public const string SendFileToArgs = "SendFileToArgs";

        /// <summary>
        /// Prevent Sleep
        /// </summary>
        public const string PreventSleep = "PreventSleep";

        /// <summary>
        /// Pause Queue on Low Disk Space
        /// </summary>
        public const string PauseOnLowDiskspace = "PauseOnLowDiskspace";

        /// <summary>
        /// Low Disk Space Warning Level in Bytes.
        /// </summary>
        public const string PauseOnLowDiskspaceLevel = "LowDiskSpaceWarningLevelInBytes";

        /// <summary>
        /// The remove punctuation.
        /// </summary>
        public const string RemovePunctuation = "RemovePunctuation";

        /// <summary>
        /// The Show Preset Panel
        /// </summary>
        public const string ShowPresetPanel = "ShowPresetPanel";

        /// <summary>
        /// The reset when done action.
        /// </summary>
        public const string ResetWhenDoneAction = "ResetWhenDoneAction";

        /// <summary>
        /// The disable lib dvd nav.
        /// </summary>
        public const string DisableLibDvdNav = "DisableLibDvdNav";

        /// <summary>
        /// The disable quick sync decoding.
        /// </summary>
        public const string DisableQuickSyncDecoding = "DisableQuickSyncDecoding";

        /// <summary>
        /// Setting indicating whether to use qsv decode for non qsv encoders
        /// </summary>
        public const string UseQSVDecodeForNonQSVEnc = "UseQSVDecodeForNonQSVEnc";

        /// <summary>
        /// The scaling mode.
        /// </summary>
        public const string ScalingMode = "ScalingMode";

        /// <summary>
        /// Preview Scan Count
        /// </summary>
        public const string PreviewScanCount = "previewScanCount";

        /// <summary>
        /// The Verbosity
        /// </summary>
        public const string Verbosity = "Verbosity";

        /// <summary>
        /// Min Title Scan Duration
        /// </summary>
        public const string MinScanDuration = "MinTitleScanDuration";

        /// <summary>
        /// Process Priority
        /// </summary>
        public const string ProcessPriority = "ProcessPriority";

        /// <summary>
        /// Save Log Directory
        /// </summary>
        public const string SaveLogToCopyDirectory = "SaveLogToCopyDirectory";

        /// <summary>
        /// Save log with video
        /// </summary>
        public const string SaveLogWithVideo = "SaveLogWithVideo";

        /// <summary>
        /// Save copy of the log to a directory
        /// </summary>
        public const string SaveLogCopyDirectory = "SaveLogCopyDirectory";

        /// <summary>
        /// The clear completed from queue.
        /// </summary>
        public const string ClearCompletedFromQueue = "ClearCompletedFromQueue";

        /// <summary>
        /// The Show Queue in-line option.
        /// </summary>
        public const string ShowQueueInline = "ShowQueueInline";

        /// <summary>
        /// Setting to allow mid-version upgrades of presets.
        /// </summary>
        public const string ForcePresetReset = "ForcePresetReset";

        /// <summary>
        /// Setting to record the expansion state of preset categories. 
        /// </summary>
        public const string PresetExpandedStateList = "PresetExpandedStateList";

        /// <summary>
        /// Setting to turn on/off the ability to show status in the title bar.
        /// </summary>
        public const string ShowStatusInTitleBar = "ShowStatusInTitleBar";

        /// <summary>
        /// Setting to turn on/off the ability to play a sound when an encode or queue is done.
        /// </summary>
        public static string PlaySoundWhenDone = "PlaySoundWhenDone";

        /// <summary>
        /// Setting to store the file to play when done.
        /// </summary>
        public static string WhenDoneAudioFile = "WhenDoneAudioFile";

        #endregion
    }
}