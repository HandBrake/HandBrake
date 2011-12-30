// --------------------------------------------------------------------------------------------------------------------
// <copyright file="IJobContextService.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the IJobContextService type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Services.Interfaces
{
    using HandBrake.ApplicationServices.Model;
    using HandBrake.ApplicationServices.Parsing;

    /// <summary>
    /// A Context service for the current Job Information
    /// </summary>
    public interface IJobContextService
    {
        /// <summary>
        /// Gets or sets CurrentTask.
        /// </summary>
        EncodeTask CurrentTask { get; set; }

        /// <summary>
        /// Gets or sets CurrentSource.
        /// </summary>
        Source CurrentSource { get; set; }
    }
}