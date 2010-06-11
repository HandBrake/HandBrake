/*  EncodeProgressEventArgs.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

namespace HandBrake.ApplicationServices
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
        public float PercentComplete { get; set; }

        /// <summary>
        /// Gets or sets CurrentFrameRate.
        /// </summary>
        public float CurrentFrameRate { get; set; }

        /// <summary>
        /// Gets or sets AverageFrameRate.
        /// </summary>
        public float AverageFrameRate { get; set; }

        /// <summary>
        /// Gets or sets EstimatedTimeLeft.
        /// </summary>
        public TimeSpan EstimatedTimeLeft { get; set; }

        /// <summary>
        /// Gets or sets Task.
        /// </summary>
        public int Task { get; set; }
    }
}
