// --------------------------------------------------------------------------------------------------------------------
// <copyright file="Working.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The working.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ApplicationServices.Interop.Json.State
{
    /// <summary>
    /// The working.
    /// </summary>
    public class Working
    {
        /// <summary>
        /// Gets or sets the hours.
        /// </summary>
        public int Hours { get; set; }

        /// <summary>
        /// Gets or sets the Pass ID.
        /// </summary>
        /// <remarks>
        /// -1: Subtitle scan
        ///  0: Encode
        ///  1: Encode first pass
        ///  2: Encode second pass
        /// </remarks>
        public int PassID { get; set; }

        /// <summary>
        /// Gets or sets the pass number (1-based).
        /// </summary>
        public int Pass { get; set; }

        /// <summary>
        /// Gets or sets the pass count.
        /// </summary>
        public int PassCount { get; set; }

        /// <summary>
        /// Gets or sets the minutes.
        /// </summary>
        public int Minutes { get; set; }

        /// <summary>
        /// Gets or sets the progress.
        /// </summary>
        public double Progress { get; set; }

        /// <summary>
        /// Gets or sets the rate.
        /// </summary>
        public double Rate { get; set; }

        /// <summary>
        /// Gets or sets the rate avg.
        /// </summary>
        public double RateAvg { get; set; }

        /// <summary>
        /// Gets or sets the seconds.
        /// </summary>
        public int Seconds { get; set; }

        /// <summary>
        /// Gets or sets the sequence id.
        /// </summary>
        public int SequenceID { get; set; }
    }
}