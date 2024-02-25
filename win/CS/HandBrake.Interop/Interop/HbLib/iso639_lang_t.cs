// --------------------------------------------------------------------------------------------------------------------
// <copyright file="iso639_lang_t.cs" company="HandBrake Project (https://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Interop.HbLib
{
    using System;
    using System.Runtime.InteropServices;

    [StructLayout(LayoutKind.Sequential)]
    internal struct iso639_lang_t
    {
        public IntPtr eng_name;

        public IntPtr native_name;

        [MarshalAs(UnmanagedType.LPStr)]
        public string iso639_1;

        [MarshalAs(UnmanagedType.LPStr)]
        public string iso639_2;

        [MarshalAs(UnmanagedType.LPStr)]
        public string iso639_2b;
    }
}
