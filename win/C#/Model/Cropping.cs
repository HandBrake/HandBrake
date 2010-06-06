/*  Cropping.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

namespace Handbrake.Model
{
    /// <summary>
    /// Cropping T B L R
    /// </summary>
    public class Cropping
    {
        /// <summary>
        /// Gets or sets Top.
        /// </summary>
        public int Top { get; set; }

        /// <summary>
        /// Gets or sets Bottom.
        /// </summary>
        public int Bottom { get; set; }

        /// <summary>
        /// Gets or sets Left.
        /// </summary>
        public int Left { get; set; }

        /// <summary>
        /// Gets or sets Right.
        /// </summary>
        public int Right { get; set; }

        /// <summary>
        /// Create a cropping object
        /// </summary>
        /// <param name="top">
        /// The top.
        /// </param>
        /// <param name="bottom">
        /// The bottom.
        /// </param>
        /// <param name="left">
        /// The left.
        /// </param>
        /// <param name="right">
        /// The right.
        /// </param>
        /// <returns>
        /// A Cropping object
        /// </returns>
        public static Cropping CreateCroppingObject(int top, int bottom, int left, int right)
        {
            return new Cropping { Top = top, Bottom = bottom, Left = left, Right = right };
        }
    }
}
