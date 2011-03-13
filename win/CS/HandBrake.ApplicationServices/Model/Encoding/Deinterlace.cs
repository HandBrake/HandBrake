/*  Deinterlace.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

namespace HandBrake.ApplicationServices.Model.Encoding
{
    /// <summary>
    /// The Deinterlace Filter
    /// </summary>
    public enum Deinterlace
    {
        Off = 0,
        Fast,
        Slow,
        Slower,
        Slowest,
        Custom
    }
}
