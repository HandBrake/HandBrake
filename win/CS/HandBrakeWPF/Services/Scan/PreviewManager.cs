// --------------------------------------------------------------------------------------------------------------------
// <copyright file="PreviewManager.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   A class for manipulating preview images.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Services.Scan
{
    using System.Drawing;

    using HandBrake.Interop.Interop.Interfaces.Model.Picture;

    public class PreviewManager
    {
        public static Bitmap RenderCropBorder(Bitmap srcImage, Cropping crop)
        {
            Bitmap bmp = new Bitmap(srcImage.Width, srcImage.Height);
            Graphics g = Graphics.FromImage(bmp);

            Brush brush = new SolidBrush(Color.FromArgb(64, 255, 0, 0));
            g.DrawImage(srcImage, 0, 0);
            g.DrawLine(new Pen(brush, crop.Top), new Point(0, 0), new Point(srcImage.Width, 0)); // Top
            g.DrawLine(new Pen(brush, crop.Top), new Point(0, srcImage.Height), new Point(srcImage.Width, srcImage.Height)); // Bottom
            g.DrawLine(new Pen(brush, crop.Left), new Point(0, 0), new Point(0, srcImage.Height)); // Left
            g.DrawLine(new Pen(brush, crop.Right), new Point(srcImage.Width, 0), new Point(srcImage.Width, srcImage.Height)); // Right

            return bmp;
        }
    }
}
