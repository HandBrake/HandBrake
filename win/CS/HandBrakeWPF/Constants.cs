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

        public const string Appcast64Arm = "https://handbrake.fr/appcast.arm64.xml";
        public const string AppcastUnstable64Arm = "https://handbrake.fr/appcast_unstable.arm64.xml";

        /* Languages */
        public const string Any = "(Any)";
        public const string Undefined = "und";

        /* Auto-name Constants */
        public const string Chapters = "{chapters}";
        public const string Title = "{title}";
        public const string QualityBitrate = "{quality_bitrate}";
        public const string Date = "{date}";
        public const string Time = "{time}";
        public const string CreationDate = "{creation-date}";
        public const string CreationTime = "{creation-time}";
        public const string ModificationDate = "{modification-date}";
        public const string ModificationTime = "{modification-time}";
        public const string Preset = "{preset}"; 
        public const string QualityType = "{quality_type}";
        public const string EncoderBitDepth = "{encoder_bit_depth}";

        /* Auto-name Path Constants */
        public const string Source = "{source}";
        public const string SourcePath = "{source_path}";
        public const string SourceFolderName = "{source_folder_name}";

        public const string FileScanMru = "FileScanMru";
        public const string FileSaveMru = "FileSaveMru";
    }
}
