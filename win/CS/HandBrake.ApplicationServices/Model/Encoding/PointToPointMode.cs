/*  PointToPoint.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

namespace HandBrake.ApplicationServices.Model.Encoding
{
    /// <summary>
    /// Point to Point Mode
    /// </summary>
    public enum PointToPointMode
    {
        Chapters = 0,
        Seconds,
        Frames
    }
}
