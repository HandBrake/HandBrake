// --------------------------------------------------------------------------------------------------------------------
// <copyright file="PointToPointMode.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Point to Point Mode
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Services.Encode.Model.Models
{
    using HandBrake.Interop.Attributes;

    /// <summary>
    /// Point to Point Mode
    /// </summary>
    public enum PointToPointMode
    {
        [DisplayName("Chapters")]
        Chapters = 0,

        [DisplayName("Seconds")]
        Seconds,

        [DisplayName("Frames")]
        Frames,

        [DisplayName("Preview")]
        Preview,
    }
}
