// --------------------------------------------------------------------------------------------------------------------
// <copyright file="SourceVideoInfo.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The source framerate info.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ApplicationServices.Interop.Model
{
    /// <summary>
    /// The source framerate info.
    /// </summary>
    public class SourceVideoInfo
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="SourceVideoInfo"/> class.
        /// </summary>
        /// <param name="framerateNumerator">
        /// The framerate numerator.
        /// </param>
        /// <param name="framerateDenominator">
        /// The framerate denominator.
        /// </param>
        /// <param name="resolution">
        /// The resolution.
        /// </param>
        /// <param name="parVal">
        /// The par val.
        /// </param>
        public SourceVideoInfo(int framerateNumerator, int framerateDenominator, Size resolution, Size parVal)
        {
            this.FramerateNumerator = framerateNumerator;
            this.FramerateDenominator = framerateDenominator;
            this.Resolution = resolution;
            this.ParVal = parVal;
        }

        /// <summary>
        /// Gets the framerate numerator.
        /// </summary>
        public int FramerateNumerator { get; private set; }

        /// <summary>
        /// Gets the framerate denominator.
        /// </summary>
        public int FramerateDenominator { get; private set; }

        /// <summary>
        /// Gets or sets the resolution (width/height) of this Title
        /// </summary>
        public Size Resolution { get; set; }

        /// <summary>
        /// Gets or sets the pixel aspect ratio.
        /// </summary>
        public Size ParVal { get; set; }
    }
}
