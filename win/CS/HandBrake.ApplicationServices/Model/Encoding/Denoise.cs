/*  Denoise.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

namespace HandBrake.ApplicationServices.Model.Encoding
{
    /// <summary>
    /// The Denose Filters
    /// </summary>
    public enum Denoise
    {
        Off = 0,
        Weak,
        Medium,
        Strong,
        Custom
    }
}
