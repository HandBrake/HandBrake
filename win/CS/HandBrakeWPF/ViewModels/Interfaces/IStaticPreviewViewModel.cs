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

    using EncodeTask = Services.Encode.Model.EncodeTask;

    /// <summary>
    /// The Static Preview View Model Interface
    /// </summary>
    public interface IStaticPreviewViewModel : IViewModelBase
    {
        bool IsOpen { get; set; }

        BitmapSource PreviewImage { get; }

        void UpdatePreviewFrame(Title title, EncodeTask task, Source scannedSource);

        void PreviousPreview();

        void NextPreview();

        void SetPictureSettingsInstance(IPictureSettingsViewModel pictureSettingsViewModel);
    }
}
