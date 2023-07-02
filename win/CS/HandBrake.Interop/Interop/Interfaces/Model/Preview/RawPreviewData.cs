// --------------------------------------------------------------------------------------------------------------------
// <copyright file="RawPreviewData.cs" company="HandBrake Project (https://handbrake.fr)">
// This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Interop.Interfaces.Model.Preview
{
    public class RawPreviewData
    {
        public RawPreviewData(byte[] rawBitmapData, int strideWidth, int strideHeight, int width, int height)
        {
            this.RawBitmapData = rawBitmapData;
            this.StrideWidth = strideWidth;
            this.StrideHeight = strideHeight;
            this.Width = width;
            this.Height = height;
        }

        public byte[] RawBitmapData { get; set; }

        public int StrideWidth { get; set; }

        public int StrideHeight { get; set; }

        public int Width { get; set; }

        public int Height { get; set; }
    }
}
