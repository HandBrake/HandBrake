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

    using HandBrakeWPF.Services.Scan.Model;

    using EncodeTask = HandBrakeWPF.Services.Encode.Model.EncodeTask;

    /// <summary>
    /// The Static Preview View Model Interface
    /// </summary>
    public interface IStaticPreviewViewModel
    {
        /// <summary>
        /// The preview frame.
        /// </summary>
        /// <param name="task">
        /// The task.
        /// </param>
        /// <param name="scannedSource">
        /// The scanned Source.
        /// </param>
        void UpdatePreviewFrame(EncodeTask task, Source scannedSource);

        /// <summary>
        /// Gets or sets a value indicating whether is open.
        /// </summary>
        bool IsOpen { get; set; }

        BitmapSource PreviewImage { get; }

        void PreviousPreview();
        void NextPreview();
    }
}
