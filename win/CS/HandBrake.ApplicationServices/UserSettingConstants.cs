/*  UserSettingConstants.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr/>.
    It may be used under the terms of the GNU General Public License. */

namespace HandBrake.ApplicationServices
{
    /// <summary>
    /// Constants for the User Settings Service
    /// </summary>
    public class UserSettingConstants
    {
        /// <summary>
        /// The Verbosity
        /// </summary>
        public const string Verbosity = "Verbosity";

        /// <summary>
        /// The X264 Stepper 
        /// </summary>
        public const string X264Step = "X264Step";

        /// <summary>
        /// When Complete Action
        /// </summary>
        public const string WhenCompleteAction = "WhenCompleteAction";

        /// <summary>
        /// Growl Encodes
        /// </summary>
        public const string GrowlEncode = "GrowlEncode";

        /// <summary>
        /// Growl Queues
        /// </summary>
        public const string GrowlQueue = "GrowlQueue";

        /// <summary>
        /// Process Priority
        /// </summary>
        public const string ProcessPriority = "ProcessPriority";

        /// <summary>
        /// Prevent Sleep
        /// </summary>
        public const string PreventSleep = "PreventSleep";

        /// <summary>
        /// Show the CLI window
        /// </summary>
        public const string ShowCLI = "ShowCLI";

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
        /// HandBrakes version
        /// </summary>
        public const string HandBrakeVersion = "HandBrakeVersion";

        /// <summary>
        /// HandBrakes build
        /// </summary>
        public const string HandBrakeBuild = "HandBrakeBuild";

        /// <summary>
        /// HandBrakes build
        /// </summary>
        public const string HandBrakePlatform = "HandBrakePlatform";


        /// <summary>
        /// HandBrakes CLI Exe SHA1 Hash
        /// </summary>
        public const string HandBrakeExeHash = "HandBrakeExeHash";

        /// <summary>
        /// The Instance Id
        /// </summary>
        public const string InstanceId = "InstanceId";

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

        public const string MinScanDuration = "MinTitleScanDuration";
    }
}
