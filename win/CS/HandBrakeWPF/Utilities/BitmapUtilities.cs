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
    using System;
    using System.Diagnostics;
    using System.Drawing;
    using System.Drawing.Imaging;
    using System.IO;
    using System.Runtime.InteropServices;
    using System.Windows.Media.Imaging;

    using HandBrake.Interop.Interop.Interfaces.Model.Preview;

    /// <summary>
    /// The bitmap utilities.
    /// </summary>
    public class BitmapUtilities
    {
        public static BitmapImage IntPtrToBitmapImage(IntPtr imagePtr, int length)
        {
            byte[] imageData = new byte[length];

            Marshal.Copy(imagePtr, imageData, 0, length);

            using (MemoryStream memoryStream = new MemoryStream(imageData))
            {
                BitmapImage bitmapImage = new BitmapImage();
                bitmapImage.BeginInit();
                bitmapImage.StreamSource = memoryStream;
                bitmapImage.EndInit();
                return bitmapImage;
            }
        }

        public static BitmapImage ConvertToBitmapImage(MemoryStream memoryStream)
        {
            var wpfBitmap = new BitmapImage();
            wpfBitmap.BeginInit();
            wpfBitmap.CacheOption = BitmapCacheOption.OnLoad;
            wpfBitmap.StreamSource = memoryStream;
            wpfBitmap.EndInit();
            wpfBitmap.Freeze();

            return wpfBitmap;
        }
        
        /// <summary>
        /// Convert a Bitmap to a BitmapImagetype.
        /// </summary>
        /// <param name="bitmap">
        /// The bitmap.
        /// </param>
        /// <returns>
        /// The <see cref="BitmapImage"/>.
        /// </returns>
        public static BitmapImage ConvertToBitmapImage(Bitmap bitmap)
        {
            // Create a Bitmap Image for display.
            using (var memoryStream = new MemoryStream())
            {
                try
                {
                    bitmap.Save(memoryStream, ImageFormat.Bmp);
                }
                finally
                {
                    bitmap.Dispose();
                }

                var wpfBitmap = new BitmapImage();
                wpfBitmap.BeginInit();
                wpfBitmap.CacheOption = BitmapCacheOption.OnLoad;
                wpfBitmap.StreamSource = memoryStream;
                wpfBitmap.EndInit();
                wpfBitmap.Freeze();

                return wpfBitmap;
            }
        }

        public static Bitmap ConvertByteArrayToBitmap(RawPreviewData previewData)
        {
            var bitmap = new Bitmap(previewData.Width, previewData.Height);
            
            BitmapData bitmapData = bitmap.LockBits(new Rectangle(0, 0, previewData.Width, previewData.Height), ImageLockMode.WriteOnly, PixelFormat.Format32bppRgb);

            IntPtr targetPtr = bitmapData.Scan0; // Pointer to the first pixel.
            try
            {
                for (int i = 0; i < previewData.Height; i++)
                {
                    Marshal.Copy(previewData.RawBitmapData, i * previewData.StrideWidth, targetPtr, previewData.Width * 4);
                    targetPtr = IntPtr.Add(targetPtr, previewData.Width * 4);
                }

                bitmap.UnlockBits(bitmapData);
            }
            catch (Exception exc)
            {
                Debug.WriteLine(exc);
            }

            return bitmap;
        }
    }
}
