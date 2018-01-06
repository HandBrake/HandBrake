// --------------------------------------------------------------------------------------------------------------------
// <copyright file="BitmapUtilities.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the BitmapUtilities type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Utilities
{
    using System.IO;
    using System.Windows.Media.Imaging;

    /// <summary>
    /// The bitmap utilities.
    /// </summary>
    public class BitmapUtilities
    {
        /// <summary>
        /// Convert a Bitmap to a BitmapImagetype.
        /// </summary>
        /// <param name="bitmap">
        /// The bitmap.
        /// </param>
        /// <returns>
        /// The <see cref="BitmapImage"/>.
        /// </returns>
        public static BitmapImage ConvertToBitmapImage(MemoryStream bitmap)
        {
            // Create a Bitmap Image for display.
            using (bitmap)
            {
                var wpfBitmap = new BitmapImage();
                wpfBitmap.BeginInit();
                wpfBitmap.CacheOption = BitmapCacheOption.OnLoad;
                wpfBitmap.StreamSource = bitmap;
                wpfBitmap.EndInit();
                wpfBitmap.Freeze();

                return wpfBitmap;
            }
        }
    }
}