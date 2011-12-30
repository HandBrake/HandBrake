// --------------------------------------------------------------------------------------------------------------------
// <copyright file="JobContextService.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the JobContextService type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Services
{
    using HandBrake.ApplicationServices.Model;
    using HandBrake.ApplicationServices.Parsing;

    using HandBrakeWPF.Services.Interfaces;

    /// <summary>
    /// A Context service for the current Job Information.
    /// </summary>
    public class JobContextService : IJobContextService
    {
        /// <summary>
        /// Gets or sets CurrentTask.
        /// </summary>
        public EncodeTask CurrentTask { get; set; }

        /// <summary>
        /// Gets or sets CurrentSource.
        /// </summary>
        public Source CurrentSource { get; set; }
    }
}
