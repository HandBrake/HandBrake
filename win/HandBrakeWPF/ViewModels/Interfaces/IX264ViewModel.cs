// --------------------------------------------------------------------------------------------------------------------
// <copyright file="IX264ViewModel.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the IX264ViewModel type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.ViewModels.Interfaces
{
    using HandBrake.ApplicationServices.Interop.Model.Encoding;

    /// <summary>
    /// The Advanced View Model Interface
    /// </summary>
    public interface IX264ViewModel : ITabInterface
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
