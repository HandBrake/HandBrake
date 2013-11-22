// --------------------------------------------------------------------------------------------------------------------
// <copyright file="IStaticPreviewViewModel.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The Static Preview View Model Interface
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.ViewModels.Interfaces
{
    using System.Windows.Media.Imaging;

    using HandBrake.ApplicationServices.Model;

    /// <summary>
    /// The Static Preview View Model Interface
    /// </summary>
    public interface IStaticPreviewViewModel
    {
        /// <summary>
        /// The preview frame.
        /// </summary>
        /// <param name="image">
        /// The image.
        /// </param>
        /// <param name="task">
        /// The task.
        /// </param>
        void PreviewFrame(BitmapImage image, EncodeTask task);
    }
}
