// --------------------------------------------------------------------------------------------------------------------
// <copyright file="Cropping.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the Cropping type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Model
{
	public class Cropping
	{
        /// <summary>
        /// Initializes a new instance of the <see cref="Cropping"/> class. 
        /// </summary>
        public Cropping()
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="Cropping"/> class. 
        /// </summary>
        /// <param name="top">
        /// The Top Value
        /// </param>
        /// <param name="bottom">
        /// The Bottom Value
        /// </param>
        /// <param name="left">
        /// The Left Value
        /// </param>
        /// <param name="right">
        /// The Right Value
        /// </param>
        public Cropping(int top, int bottom, int left, int right)
        {
            this.Top = top;
            this.Bottom = bottom;
            this.Left = left;
            this.Right = right;
        }

		public int Top { get; set; }
		public int Bottom { get; set; }
		public int Left { get; set; }
		public int Right { get; set; }

		public Cropping Clone()
		{
			return new Cropping
			{
				Top = this.Top,
				Bottom = this.Bottom,
				Left = this.Left,
				Right = this.Right
			};
		}
	}
}
