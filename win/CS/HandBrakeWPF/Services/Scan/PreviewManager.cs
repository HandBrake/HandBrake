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
    using System;
    using System.Diagnostics;
    using System.Drawing;

    using HandBrake.Interop.Interop.Interfaces.Model.Picture;

    using HandBrakeWPF.Properties;

    public class PreviewManager
    {
        public static Bitmap RenderCropBorder(Bitmap srcImage, Cropping crop)
        {
            try
            {
                Bitmap bmp = new Bitmap(srcImage.Width, srcImage.Height);

                using (Graphics g = Graphics.FromImage(bmp))
                {
                    g.DrawImage(srcImage, 0, 0);

                    int left = Math.Max(0, Math.Min(crop.Left, srcImage.Width));
                    int top = Math.Max(0, Math.Min(crop.Top, srcImage.Height));
                    int right = Math.Max(0, Math.Min(crop.Right, srcImage.Width));
                    int bottom = Math.Max(0, Math.Min(crop.Bottom, srcImage.Height));

                    int width = Math.Max(0, srcImage.Width - left - right);
                    int height = Math.Max(0, srcImage.Height - top - bottom);

                    using (Brush tint = new SolidBrush(Color.FromArgb(128, 211, 211, 211)))
                    {
                        if (top > 0)
                        {
                            g.FillRectangle(tint, 0, 0, srcImage.Width, top);
                        }

                        if (bottom > 0)
                        {
                            g.FillRectangle(tint, 0, srcImage.Height - bottom, srcImage.Width, bottom);
                        }

                        if (left > 0 && height > 0)
                        {
                            g.FillRectangle(tint, 0, top, left, height);
                        }

                        if (right > 0 && height > 0)
                        {
                            g.FillRectangle(tint, srcImage.Width - right, top, right, height);
                        }
                    }

                    if (width > 0 && height > 0)
                    {
                        float dashLength = 6f;
                        float gapLength = 4f;
                        int x1 = left;
                        int y1 = top;
                        int x2 = left + width - 1;
                        int y2 = top + height - 1;
                        bool useDark = true;

                        using (Pen darkPen = new Pen(Color.FromArgb(220, 64, 64, 64), 1f))
                        using (Pen lightPen = new Pen(Color.FromArgb(220, 192, 192, 192), 1f))
                        {
                            if (top > 0)
                            {
                                float topY = y1 - 1;
                                DrawAlternatingDashedLine(
                                    g,
                                    darkPen,
                                    lightPen,
                                    new PointF(x1, topY),
                                    new PointF(x2, topY),
                                    dashLength,
                                    gapLength,
                                    ref useDark); // Top
                            }

                            if (right > 0)
                            {
                                float rightX = x2 + 1;
                                DrawAlternatingDashedLine(
                                    g,
                                    darkPen,
                                    lightPen,
                                    new PointF(rightX, y1),
                                    new PointF(rightX, y2),
                                    dashLength,
                                    gapLength,
                                    ref useDark); // Right
                            }

                            if (bottom > 0)
                            {
                                float bottomY = y2 + 1;
                                DrawAlternatingDashedLine(
                                    g,
                                    darkPen,
                                    lightPen,
                                    new PointF(x2, bottomY),
                                    new PointF(x1, bottomY),
                                    dashLength,
                                    gapLength,
                                    ref useDark); // Bottom
                            }

                            if (left > 0)
                            {
                                float leftX = x1 - 1;
                                DrawAlternatingDashedLine(
                                    g,
                                    darkPen,
                                    lightPen,
                                    new PointF(leftX, y2),
                                    new PointF(leftX, y1),
                                    dashLength,
                                    gapLength,
                                    ref useDark); // Left
                            }
                        }
                    }
                }

                return bmp;
            }
            catch (Exception exc)
            {
                Debug.WriteLine(exc);
                return CreateFallbackBitmap();
            }
        }

        private static void DrawAlternatingDashedLine(
            Graphics g,
            Pen darkPen,
            Pen lightPen,
            PointF start,
            PointF end,
            float dashLength,
            float gapLength,
            ref bool useDark)
        {
            float dx = end.X - start.X;
            float dy = end.Y - start.Y;
            float length = (float)Math.Sqrt((dx * dx) + (dy * dy));

            if (length <= 0)
            {
                return;
            }

            float ux = dx / length;
            float uy = dy / length;
            float position = 0f;

            while (position < length)
            {
                float dashEnd = Math.Min(position + dashLength, length);

                PointF dashStartPoint = new PointF(start.X + (ux * position), start.Y + (uy * position));
                PointF dashEndPoint = new PointF(start.X + (ux * dashEnd), start.Y + (uy * dashEnd));

                g.DrawLine(useDark ? darkPen : lightPen, dashStartPoint, dashEndPoint);
                useDark = !useDark;

                position = dashEnd + gapLength;
            }
        }


        private static Bitmap CreateFallbackBitmap()
        {
            Bitmap fallback = new Bitmap(1280, 720);

            using (Graphics g = Graphics.FromImage(fallback))
            using (Brush background = new SolidBrush(Color.LightGray))
            using (Brush textBrush = new SolidBrush(Color.DimGray))
            using (Font font = new Font(FontFamily.GenericSansSerif, 48, FontStyle.Bold, GraphicsUnit.Pixel))
            using (StringFormat format = new StringFormat { Alignment = StringAlignment.Center, LineAlignment = StringAlignment.Center })
            {
                g.FillRectangle(background, 0, 0, fallback.Width, fallback.Height);
                g.DrawString(Resources.StaticPreviewView_ErrorUnableToGenerateImage, font, textBrush, new RectangleF(0, 0, fallback.Width, fallback.Height), format);
            }

            return fallback;
        }
    }
}
