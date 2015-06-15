// --------------------------------------------------------------------------------------------------------------------
// <copyright file="EncodeProgressEventArgs.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the EncodeProgressEventArgs type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ApplicationServices.Interop.EventArgs
{
    using System;

    /// <summary>s
    /// Encode Progress Event Args
    /// </summary>
    public class EncodeProgressEventArgs : EventArgs
    {
        /// <summary>
        /// Gets or sets FractionComplete.
        /// </summary>
        public double FractionComplete { get; set; }

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
		/// Gets or sets the pass ID.
		/// </summary>
		/// <remarks>
		/// -1: Subtitle scan
		///  0: Encode
		///  1: Encode first pass
		///  2: Encode second pass
		/// </remarks>
		public int PassId { get; set; }

        /// <summary>
        /// Gets or sets the current encoding pass. (1-based)
        /// </summary>
        public int Pass { get; set; }

        /// <summary>
        /// Gets or sets the pass count.
        /// </summary>
        public int PassCount { get; set; }
    }
}
