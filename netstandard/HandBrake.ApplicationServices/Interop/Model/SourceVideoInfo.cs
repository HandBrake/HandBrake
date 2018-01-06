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
        /// <param name="resolution">
        /// The resolution.
        /// </param>
        /// <param name="parVal">
        /// The par val.
        /// </param>
        public SourceVideoInfo(Size resolution, Size parVal)
        {
            this.Resolution = resolution;
            this.ParVal = parVal;
        }

        /// <summary>
        /// Gets the resolution (width/height) of this Title
        /// </summary>
        public Size Resolution { get; private set; }

        /// <summary>
        /// Gets the pixel aspect ratio.
        /// </summary>
        public Size ParVal { get; private set; }
    }
}
