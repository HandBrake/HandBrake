// --------------------------------------------------------------------------------------------------------------------
// <copyright file="CoverArtImage.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Model
{
    using System;
    using System.Windows.Media.Imaging;

    using HandBrake.Interop.Interop.Json.Shared;
    using HandBrakeWPF.ViewModels;

    public class CoverArtImage : PropertyChangedBase
    {
        private BitmapImage activeImage;

        private bool enabled;

        public CoverArtImage(CoverArt coverArt)
        {
            this.CoverArt = coverArt;

            if (coverArt == null)
            {
                this.Image = new BitmapImage(new Uri("pack://application:,,,/Views/Images/EmptyCoverArt.png"));
                this.IsNoCoverArt = true;
            }
            else
            {
                this.Enabled = true;
                this.IsNoCoverArt = false;
            }
        }

        public bool Enabled
        {
            get => this.enabled;
            set
            {
                this.enabled = value;
                if (value)
                {
                    this.Image = this.activeImage;
                }
                else
                {
                    this.activeImage = this.Image;
                    this.Image = new BitmapImage(new Uri("pack://application:,,,/Views/Images/EmptyCoverArt.png"));
                }

                this.NotifyOfPropertyChange(() => this.Image);
            }
        }

        public CoverArt CoverArt { get; set; }

        public BitmapImage Image { get; set; }

        public bool IsNoCoverArt { get; set; }
    }
}
