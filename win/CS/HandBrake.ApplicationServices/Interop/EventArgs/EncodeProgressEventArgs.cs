﻿// --------------------------------------------------------------------------------------------------------------------
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
        /// Initializes a new instance of the <see cref="EncodeProgressEventArgs"/> class.
        /// </summary>
        /// <param name="fractionComplete">
        /// The fraction complete.
        /// </param>
        /// <param name="currentFrameRate">
        /// The current frame rate.
        /// </param>
        /// <param name="averageFrameRate">
        /// The average frame rate.
        /// </param>
        /// <param name="estimatedTimeLeft">
        /// The estimated time left.
        /// </param>
        /// <param name="passId">
        /// The pass id.
        /// </param>
        /// <param name="pass">
        /// The pass.
        /// </param>
        /// <param name="passCount">
        /// The pass count.
        /// </param>
        public EncodeProgressEventArgs(double fractionComplete, double currentFrameRate, double averageFrameRate, TimeSpan estimatedTimeLeft, int passId, int pass, int passCount)
        {
            this.FractionComplete = fractionComplete;
            this.CurrentFrameRate = currentFrameRate;
            this.AverageFrameRate = averageFrameRate;
            this.EstimatedTimeLeft = estimatedTimeLeft;
            this.PassId = passId;
            this.Pass = pass;
            this.PassCount = passCount;
        }

        /// <summary>
        /// Gets the % Complete.
        /// </summary>
        public double FractionComplete { get; private set; }

        /// <summary>
        /// Gets the Current FrameRate.
        /// </summary>
        public double CurrentFrameRate { get; private set; }

        /// <summary>
        /// Gets the Average FrameRate.
        /// </summary>
        public double AverageFrameRate { get; private set; }

        /// <summary>
        /// Gets the Estimated Time Left.
        /// </summary>
        public TimeSpan EstimatedTimeLeft { get; private set; }

        /// <summary>
        /// Gets the pass ID.
        /// </summary>
        /// <remarks>
        /// -1: Subtitle scan
        ///  0: Encode
        ///  1: Encode first pass
        ///  2: Encode second pass
        /// </remarks>
        public int PassId { get; private set; }

        /// <summary>
        /// Gets the current encoding pass. (1-based)
        /// </summary>
        public int Pass { get; private set; }

        /// <summary>
        /// Gets the pass count.
        /// </summary>
        public int PassCount { get; private set; }
    }
}
