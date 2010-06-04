// --------------------------------------------------------------------------------------------------------------------
// <copyright file="Size.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the Size type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Model
{
    public class Size
    {
        public Size(int width, int height)
        {
            this.Width = width;
            this.Height = height;
        }

        public int Width { get; set; }
        public int Height { get; set; }
    }
}
