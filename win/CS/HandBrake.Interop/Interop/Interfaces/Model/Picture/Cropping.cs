// --------------------------------------------------------------------------------------------------------------------
// <copyright file="Cropping.cs" company="HandBrake Project (https://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the Cropping type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Interop.Interfaces.Model.Picture
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
        /// <param name="cropping">
        /// The cropping.
        /// </param>
        public Cropping(Cropping cropping)
        {
            this.Top = cropping.Top;
            this.Bottom = cropping.Bottom;
            this.Left = cropping.Left;
            this.Right = cropping.Right;
            this.CropMode = cropping.CropMode;
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
        public Cropping(int top, int bottom, int left, int right, int cropMode)
        {
            this.Top = top;
            this.Bottom = bottom;
            this.Left = left;
            this.Right = right;
            this.CropMode = cropMode;
        }

        public int CropMode { get; set; }

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
