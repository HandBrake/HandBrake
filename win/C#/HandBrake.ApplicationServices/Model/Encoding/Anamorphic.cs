/*  Anamorphic.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

namespace HandBrake.ApplicationServices.Model.Encoding
{
    using System.ComponentModel;

    /// <summary>
    /// Anamorphic Mode
    /// </summary>
    public enum Anamorphic
    {
        [Description("None")]
        None = 0,
        [Description("Strict")]
        Strict,
        [Description("Loose")]
        Loose,
        [Description("Custom")]
        Custom
    }
}
