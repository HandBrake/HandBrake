// --------------------------------------------------------------------------------------------------------------------
// <copyright file="CombDetect.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the CombDetect type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.CoreLibrary.Interop.Model.Encoding
{
    using HandBrake.CoreLibrary.Attributes;

    /// <summary>
    /// The CombDetect Type.
    /// </summary>
    public enum CombDetect
    {
        [ShortName("off")]
        Off,

        [ShortName("custom")]
        Custom,

        [ShortName("default")]
        Default,

        [ShortName("permissive")]
        LessSensitive,

        [ShortName("fast")]
        Fast
    }
}
