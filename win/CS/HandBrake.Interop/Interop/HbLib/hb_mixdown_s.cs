// --------------------------------------------------------------------------------------------------------------------
// <copyright file="hb_mixdown_s.cs" company="HandBrake Project (https://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Interop.HbLib
{
    using System.Runtime.InteropServices;

    [StructLayout(LayoutKind.Sequential)]
    internal struct hb_mixdown_s
    {
        [MarshalAs(UnmanagedType.LPStr)]
        public string name;

        /// char*
        [MarshalAs(UnmanagedType.LPStr)]
        public string short_name;

        /// int
        public int amixdown;
    }
}
