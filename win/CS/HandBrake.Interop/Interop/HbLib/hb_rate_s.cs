// --------------------------------------------------------------------------------------------------------------------
// <copyright file="hb_rate_s.cs" company="HandBrake Project (https://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Interop.HbLib
{
    using System.Runtime.InteropServices;

    [StructLayout(LayoutKind.Sequential)]
    internal struct hb_rate_s
    {
        /// char*
        [MarshalAs(UnmanagedType.LPStr)]
        public string name;

        /// int
        public int rate;
    }
}
