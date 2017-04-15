// --------------------------------------------------------------------------------------------------------------------
// <copyright file="EncodeProgressEventArgs.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Encode Progress Event Args
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Services.Encode.EventArgs
{
    using System;

    /// <summary>
    /// Encode Progress Event Args
    /// </summary>
    public class EncodeProgressEventArgs : EventArgs
    {
        /// <summary>
        /// Gets or sets PercentComplete.
        /// </summary>
        public double PercentComplete { get; set; }

        /// <summary>
        /// Gets or sets CurrentFrameRate.
        /// </summary>
        public double CurrentFrameRate { get; set; }

        /// <summary>
        /// Gets or sets AverageFrameRate.
        /// </summary>
        public double AverageFrameRate { get; set; }

        /// <summary>
        /// Gets or sets EstimatedTimeLeft.
        /// </summary>
        public TimeSpan EstimatedTimeLeft { get; set; }

        /// <summary>
        /// Gets or sets Task.
        /// </summary>
        public int Task { get; set; }

        /// <summary>
        /// Gets or sets TaskCount.
        /// </summary>
        public int TaskCount { get; set; }

        /// <summary>
        /// Gets or sets ElapsedTime.
        /// </summary>
        public TimeSpan ElapsedTime { get; set; }

        /// <summary>
        ///  Gets or sets PassId.
        /// </summary>
        /// <remarks>
        /// -1: Subtitle scan
        ///  0: Encode
        ///  1: Encode first pass
        ///  2: Encode second pass
        /// </remarks>
        public int PassId { get; set; }

        /// <summary>
        /// Gets a value indicating that we are in the muxing process.
        /// </summary>
        public bool IsMuxing { get; set; }

        /// <summary>
        /// Gets a value indicating that we are in the searching process.
        /// </summary>
        public bool IsSearching { get; set; }

        /// <summary>
        /// Gets a value indicating that we are doing a subtitle scan pass.
        /// </summary>
        public bool IsSubtitleScan
        {
            get
            {
                if (this.PassId == -1)
                {
                    return true;
                }

                return false;
            }
        }
    }
}
