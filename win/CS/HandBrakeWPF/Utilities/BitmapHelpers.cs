// --------------------------------------------------------------------------------------------------------------------
// <copyright file="DelayedActionProcessor.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   An Action processor that supports queueing/delayed action processing.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Utilities
{
    using System.Windows.Media;
    using System.Windows.Media.Imaging;

    public class BitmapHelpers
    {
        public static BitmapSource CreateTransformedBitmap(BitmapSource source, int rotation, bool flip)
        {
            if ((rotation == 0) && !flip)
            {
                return source;
            }

            TransformedBitmap transformedBitmap = new TransformedBitmap();
            transformedBitmap.BeginInit();
            transformedBitmap.Source = source;

            var transformGroup = new TransformGroup();
            transformGroup.Children.Add(new ScaleTransform(1, flip ? -1 : 1));
            transformGroup.Children.Add(new RotateTransform(rotation));

            transformedBitmap.Transform = transformGroup;
            transformedBitmap.EndInit();
            transformedBitmap.Freeze();

            return (BitmapSource)transformedBitmap;
        }
    }
}
