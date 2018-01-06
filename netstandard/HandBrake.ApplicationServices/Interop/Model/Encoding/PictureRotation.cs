// --------------------------------------------------------------------------------------------------------------------
// <copyright file="PictureRotation.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Possible picture rotations.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ApplicationServices.Interop.Model.Encoding
{
    /// <summary>
    /// Possible picture rotations.
    /// </summary>
    public enum PictureRotation
    {
        None = 0,
        Clockwise90,
        Clockwise180,
        Clockwise270
    }
}
