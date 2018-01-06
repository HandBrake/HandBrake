// --------------------------------------------------------------------------------------------------------------------
// <copyright file="IErrorViewModel.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the IErrorViewModel type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.ViewModels.Interfaces
{
    /// <summary>
    /// The Error View Model Interface
    /// </summary>
    public interface IErrorViewModel
    {
        /// <summary>
        /// Sets Details.
        /// </summary>
        string Details { set; }

        /// <summary>
        /// Sets ErrorMessage.
        /// </summary>
        string ErrorMessage { set; }

        /// <summary>
        /// Sets Solution.
        /// </summary>
        string Solution { set; }
    }
}
