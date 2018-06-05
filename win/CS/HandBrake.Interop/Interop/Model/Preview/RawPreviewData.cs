// --------------------------------------------------------------------------------------------------------------------
// <copyright file="RawPreviewData.cs" company="HandBrake Project (http://handbrake.fr)">
// This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Interop.Model.Preview
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

        public byte[] RawBitmapData { get; }
        public int StrideWidth { get; }
        public int StrideHeight { get; }
        public int Width { get; }
        public int Height { get; }
    }
}
