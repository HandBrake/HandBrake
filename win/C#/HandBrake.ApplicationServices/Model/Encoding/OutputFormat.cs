/*  OutputFormat.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

namespace HandBrake.ApplicationServices.Model.Encoding
{
    using System.ComponentModel;

    /// <summary>
    /// The Output format.
    /// </summary>
    public enum OutputFormat
    {
        [Description("MP4")]
        Mp4,

        [Description("M4V")]
        M4V,

        [Description("MKV")]
        Mkv
    }
}
