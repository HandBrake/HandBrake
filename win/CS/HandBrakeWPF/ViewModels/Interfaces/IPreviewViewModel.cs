// --------------------------------------------------------------------------------------------------------------------
// <copyright file="IPreviewViewModel.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The Preview View Model Interface
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.ViewModels.Interfaces
{
    using HandBrake.ApplicationServices.Model;
    using HandBrake.ApplicationServices.Services.Encode.Model;

    /// <summary>
    /// The Preview View Model Interface
    /// </summary>
    public interface IPreviewViewModel
    {
        /// <summary>
        /// Sets Task.
        /// </summary>
        EncodeTask Task { set; }
    }
}