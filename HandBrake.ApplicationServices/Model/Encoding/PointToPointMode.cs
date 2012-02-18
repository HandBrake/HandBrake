/*  PointToPoint.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

namespace HandBrake.ApplicationServices.Model.Encoding
{
    using System.ComponentModel.DataAnnotations;

    /// <summary>
    /// Point to Point Mode
    /// </summary>
    public enum PointToPointMode
    {
        [Display(Name = "Chapters")]
        Chapters = 0,

        [Display(Name = "Seconds")]
        Seconds,

        [Display(Name = "Frames")]
        Frames,

        [Display(Name = "Preview")]
        Preview,
    }
}
