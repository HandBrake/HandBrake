// --------------------------------------------------------------------------------------------------------------------
// <copyright file="Constants.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Application Constants
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF
{
    /// <summary>
    /// Application Constants
    /// </summary>
    public class Constants
    {
        /* Appcast URLs */
        public const string Appcast64 = "https://handbrake.fr/appcast.x86_64.xml";
        public const string AppcastUnstable64 = "https://handbrake.fr/appcast_unstable.x86_64.xml";

        /* Languages */
        public const string Any = "(Any)";
        public const string Undefined = "und";

        /* Auto-name Constants */
        public const string Chapters = "{chapters}";
        public const string Title = "{title}";
        public const string Quality = "{quality}";
        public const string Date = "{date}";
        public const string Time = "{time}";
        public const string CretaionDate = "{creation-date}";
        public const string CreationTime = "{creation-time}";
        public const string Bitrate = "{bitrate}";
        public const string Preset = "{preset}";
        public const string Source = "{source}";

        public const string SourcePath = "{source_path}";
        public const string SourceFolderName = "{source_folder_name}";

        public const string FileScanMru = "FileScanMru";
        public const string FileSaveMru = "FileSaveMru";
    }
}
