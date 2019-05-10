// --------------------------------------------------------------------------------------------------------------------
// <copyright file="HBConfiguration.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   HandBrakes Configuration options
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Model
{
    public class HBConfiguration
    {
        public HBConfiguration()
        {
        }

        /// <summary>
        /// Gets or sets a value indicating whether is dvd nav disabled.
        /// </summary>
        public bool IsDvdNavDisabled { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether disable quick sync decoding.
        /// </summary>
        public bool EnableQuickSyncDecoding { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether use qsv decode for non qsv enc.
        /// </summary>
        public bool UseQSVDecodeForNonQSVEnc { get; set; }

        /// <summary>
        /// Gets or sets the scaling mode.
        /// </summary>
        public VideoScaler ScalingMode { get; set; }

        /// <summary>
        /// Gets or sets the preview scan count.
        /// </summary>
        public int PreviewScanCount { get; set; }

        /// <summary>
        /// Gets or sets the verbosity.
        /// </summary>
        public int Verbosity { get; set; }

        /// <summary>
        /// Gets or sets the min scan duration.
        /// </summary>
        public int MinScanDuration { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether save log to copy directory.
        /// </summary>
        public bool SaveLogToCopyDirectory { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether save log with video.
        /// </summary>
        public bool SaveLogWithVideo { get; set; }

        /// <summary>
        /// Gets or sets the save log copy directory.
        /// </summary>
        public string SaveLogCopyDirectory { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether we use in-process or out-of-process encoding.
        /// </summary>
        public bool RemoteServiceEnabled { get; set; }

        /// <summary>
        /// Gets or sets a value indicating what port the worker process is to use.
        /// </summary>
        public int RemoteServicePort { get; set; }

        public bool EnableVceEncoder { get; set; }

        public bool EnableNvencEncoder { get; set; }

        public bool EnableQsvEncoder { get; set; }
    }
}
