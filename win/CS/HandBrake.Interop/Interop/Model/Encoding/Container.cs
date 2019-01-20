// --------------------------------------------------------------------------------------------------------------------
// <copyright file="Container.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the Container type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Interop.Model.Encoding
{
    using System;

    using HandBrake.Interop.Attributes;

    /// <summary>
    /// The container.
    /// </summary>
    [Flags]
    public enum Container
    {
        None = 0x0,

        [DisplayName("MP4")]
        MP4,
        [DisplayName("MKV")]
        MKV,
        [DisplayName("WebM")]
        WebM
    }
}
