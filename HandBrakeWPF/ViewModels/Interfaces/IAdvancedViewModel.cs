// --------------------------------------------------------------------------------------------------------------------
// <copyright file="IAdvancedViewModel.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the IAdvancedViewModel type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.ViewModels.Interfaces
{
    using HandBrake.Interop.Model.Encoding;

    /// <summary>
    /// The Advanced View Model Interface
    /// </summary>
    public interface IAdvancedViewModel : ITabInterface
    {
        /// <summary>
        /// Set the currently selected encoder.
        /// </summary>
        /// <param name="encoder">
        /// The Video Encoder.
        /// </param>
        void SetEncoder(VideoEncoder encoder);

        /// <summary>
        /// Clear out the settings.
        /// </summary>
        void Clear();
    }
}
