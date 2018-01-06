// --------------------------------------------------------------------------------------------------------------------
// <copyright file="Cropping.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the Cropping type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ApplicationServices.Interop.Model
{
    /// <summary>
    /// The Cropping Model
    /// </summary>
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
        /// Copy Constructor
        /// </summary>
        /// <param name="croping">
        /// The croping.
        /// </param>
        public Cropping(Cropping croping)
        {
            this.Top = croping.Top;
            this.Bottom = croping.Bottom;
            this.Left = croping.Left;
            this.Right = croping.Right;
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
        /// Clone this model
        /// </summary>
        /// <returns>
        /// A Cloned copy
        /// </returns>
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
