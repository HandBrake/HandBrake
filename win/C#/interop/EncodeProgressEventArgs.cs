// --------------------------------------------------------------------------------------------------------------------
// <copyright file="EncodeProgressEventArgs.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the EncodeProgressEventArgs type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop
{
    using System;

    /// <summary>
    /// Encode Progress Event Arguments
    /// </summary>
    public class EncodeProgressEventArgs : EventArgs
    {
        /// <summary>
        /// Gets or sets FractionComplete.
        /// </summary>
        public float FractionComplete { get; set; }

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
        /// Gets or sets Pass.
        /// </summary>
        public int Pass { get; set; }
    }
}
