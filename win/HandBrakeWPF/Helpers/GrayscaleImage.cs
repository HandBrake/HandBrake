// --------------------------------------------------------------------------------------------------------------------
// <copyright file="GrayscaleImage.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Extend the Image Class to support a grayscale mode.
//   Usage: local:AutoGreyableImage Source="Image.png"
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Helpers
{
    using System;
    using System.Windows;
    using System.Windows.Controls;
    using System.Windows.Media;
    using System.Windows.Media.Imaging;

    /// <summary>
    /// Extend the Image Class to support a grayscale mode.
    /// </summary>
    public class GrayscaleImage : Image
    {
        /// <summary>
        /// Initializes static members of the <see cref="GrayscaleImage"/> class. 
        /// Usage: local:AutoGreyableImage Source="Image.png"
        /// </summary>
        static GrayscaleImage()
        {
            // Override the metadata of the IsEnabled property.
            IsEnabledProperty.OverrideMetadata(typeof(GrayscaleImage), new FrameworkPropertyMetadata(true, IsEnabledPropertyChanged));
        }

        /// <summary>
        /// The is enabled property changed.
        /// When this changes, grayscale the image when false, leave with colour when true.
        /// </summary>
        /// <param name="source">
        /// The source.
        /// </param>
        /// <param name="args">
        /// The args.
        /// </param>
        private static void IsEnabledPropertyChanged(DependencyObject source, DependencyPropertyChangedEventArgs args)
        {
            var sourceImage = source as GrayscaleImage;
            if (sourceImage != null)
            {
                if (!Convert.ToBoolean(args.NewValue))
                {
                    var bitmapImage = new BitmapImage(new Uri(sourceImage.Source.ToString()));
                    sourceImage.Source = new FormatConvertedBitmap(bitmapImage, PixelFormats.Gray32Float, null, 0);
                    sourceImage.OpacityMask = new ImageBrush(bitmapImage);
                }
                else
                {
                    sourceImage.Source = ((FormatConvertedBitmap)sourceImage.Source).Source;
                    sourceImage.OpacityMask = null;
                }
            }
        }
    }
}
