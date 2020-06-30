// --------------------------------------------------------------------------------------------------------------------
// <copyright file="StartupOptions.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The Castle Bootstrapper
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Startup
{
    using System.Collections.Generic;

    /// <summary>
    /// The startup options.
    /// </summary>
    public class StartupOptions
    {
        /// <summary>
        /// Gets or sets a value indicating whether auto restart queue.
        /// </summary>
        public static bool AutoRestartQueue { get; set; }

        public static List<string> QueueRecoveryIds { get; set; }
    }
}
