// --------------------------------------------------------------------------------------------------------------------
// <copyright file="StaticPreviewViewModel.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The Static Preview View Model
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.ViewModels
{
    using System.Windows.Media.Imaging;

    using HandBrakeWPF.ViewModels.Interfaces;

    /// <summary>
    /// The Static Preview View Model
    /// </summary>
    public class StaticPreviewViewModel : ViewModelBase, IStaticPreviewViewModel
    {
        /// <summary>
        /// The preview image.
        /// </summary>
        private BitmapImage previewImage;

        /// <summary>
        /// Gets or sets the preview image.
        /// </summary>
        public BitmapImage PreviewImage
        {
            get
            {
                return this.previewImage;
            }
            set
            {
                if (Equals(value, this.previewImage))
                {
                    return;
                }

                this.previewImage = value;
                this.NotifyOfPropertyChange(() => this.PreviewImage);
            }
        }

        /// <summary>
        /// The preview frame.
        /// </summary>
        /// <param name="image">
        /// The image.
        /// </param>
        public void PreviewFrame(BitmapImage image)
        {
            this.PreviewImage = image;
        }
    }
}
