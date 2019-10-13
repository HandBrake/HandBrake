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

    using HandBrakeWPF.Properties;

    /// <summary>
    /// Point to Point Mode
    /// </summary>
    public enum PointToPointMode
    {
        [DisplayName(typeof(Resources), "PointToPointMode_Chapters")]
        [ShortName("chapter")]
        Chapters = 0,

        [DisplayName(typeof(Resources), "PointToPointMode_Seconds")]
        [ShortName("time")]
        Seconds,

        [DisplayName(typeof(Resources), "PointToPointMode_Frames")]
        [ShortName("frame")]
        Frames,

        [DisplayName(typeof(Resources), "PointToPointMode_Preview")]
        [ShortName("preview")]
        Preview,
    }
}
