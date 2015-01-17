// --------------------------------------------------------------------------------------------------------------------
// <copyright file="HBConfiguration.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   HandBrakes Configuration options
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ApplicationServices.Model
{
    /// <summary>
    /// HandBrakes configuration options
    /// </summary>
    public class HBConfiguration
    {
        /// <summary>
        /// Gets or sets a value indicating whether is dvd nav disabled.
        /// </summary>
        public bool IsDvdNavDisabled { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether disable quick sync decoding.
        /// </summary>
        public bool DisableQuickSyncDecoding { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether enable dxva.
        /// </summary>
        public bool EnableDxva { get; set; }

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
        /// Gets or sets the process priority.
        /// </summary>
        public string ProcessPriority { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether save log to copy directory.
        /// </summary>
        public bool SaveLogToCopyDirectory { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether save log with video.
        /// </summary>
        public bool SaveLogWithVideo{ get; set; }

        /// <summary>
        /// Gets or sets the save log copy directory.
        /// </summary>
        public string SaveLogCopyDirectory { get; set; }
    }
}
