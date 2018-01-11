// --------------------------------------------------------------------------------------------------------------------
// <copyright file="ImageData.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   A wrapper for image data.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.CoreLibrary.Model
{
    using System;
    using System.Drawing;
    using System.Drawing.Imaging;
    using System.IO;

    /// <summary>
    /// A wrapper for image data.
    /// </summary>
    public class ImageData : IDisposable
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="ImageData"/> class.
        /// </summary>
        /// <param name="bitmap">Bitmap instance</param>
        internal ImageData(Bitmap bitmap)
        {
            this.Width = bitmap.Width;
            this.Height = bitmap.Height;

            this.Stream = new MemoryStream();
            try
            {
                bitmap.Save(this.Stream, ImageFormat.Bmp);
            }
            catch
            {
            }
        }

        /// <summary>
        /// Gets the Image Data Stream.
        /// </summary>
        public MemoryStream Stream { get; }

        /// <summary>
        /// Gets the Width of the Image
        /// </summary>
        public int Width { get; }

        /// <summary>
        /// Gets the Height of the Image
        /// </summary>
        public int Height { get; }

        /// <summary>
        /// Disposes of the MemoryStream.
        /// </summary>
        public void Dispose()
        {
            this.Stream?.Dispose();
        }
    }
}