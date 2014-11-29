// --------------------------------------------------------------------------------------------------------------------
// <copyright file="EncodeProgressEventArgs.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Encode Progress Event Args
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ApplicationServices.Services.Encode.EventArgs
{
    using System;
    using System.Runtime.Serialization;

    /// <summary>
    /// Encode Progress Event Args
    /// </summary>
    [DataContract]
    public class EncodeProgressEventArgs : EventArgs
    {
        /// <summary>
        /// Gets or sets PercentComplete.
        /// </summary>
        [DataMember]
        public float PercentComplete { get; set; }

        /// <summary>
        /// Gets or sets CurrentFrameRate.
        /// </summary>
        [DataMember]
        public float CurrentFrameRate { get; set; }

        /// <summary>
        /// Gets or sets AverageFrameRate.
        /// </summary>
        [DataMember]
        public float AverageFrameRate { get; set; }

        /// <summary>
        /// Gets or sets EstimatedTimeLeft.
        /// </summary>
        [DataMember]
        public TimeSpan EstimatedTimeLeft { get; set; }

        /// <summary>
        /// Gets or sets Task.
        /// </summary>
        [DataMember]
        public int Task { get; set; }

        /// <summary>
        /// Gets or sets TaskCount.
        /// </summary>
        [DataMember]
        public int TaskCount { get; set; }

        /// <summary>
        /// Gets or sets ElapsedTime.
        /// </summary>
        [DataMember]
        public TimeSpan ElapsedTime { get; set; }
    }
}
