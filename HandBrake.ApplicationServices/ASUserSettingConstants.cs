// --------------------------------------------------------------------------------------------------------------------
// <copyright file="ASUserSettingConstants.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The User Setting Constants
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ApplicationServices
{
    /// <summary>
    /// Constants for the User Settings Service
    /// </summary>
    public class ASUserSettingConstants
    {
        /// <summary>
        /// The Verbosity
        /// </summary>
        public const string Verbosity = "Verbosity";

        /// <summary>
        /// When Complete Action
        /// </summary>
        public const string WhenCompleteAction = "WhenCompleteAction";

        /// <summary>
        /// Process Priority
        /// </summary>
        public const string ProcessPriority = "ProcessPriority";

        /// <summary>
        /// Prevent Sleep
        /// </summary>
        public const string PreventSleep = "PreventSleep";

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
        /// HandBrakes build
        /// </summary>
        public const string HandBrakeBuild = "HandBrakeBuild";

        /// <summary>
        /// Disable Libdvdnav
        /// </summary>
        public const string DisableLibDvdNav = "DisableLibDvdNav";

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
        /// Min Title Scan Duration
        /// </summary>
        public const string MinScanDuration = "MinTitleScanDuration";

        /// <summary>
        /// Preview Scan Count
        /// </summary>
        public const string PreviewScanCount = "previewScanCount";

        /// <summary>
        /// Clear completed items from the queue automatically.
        /// </summary>
        public const string ClearCompletedFromQueue = "ClearCompletedFromQueue";
    }
}
