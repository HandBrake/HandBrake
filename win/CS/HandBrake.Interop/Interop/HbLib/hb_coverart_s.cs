// --------------------------------------------------------------------------------------------------------------------
// <copyright file="hb_coverart_s.cs" company="HandBrake Project (https://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Interop.HbLib
{
    using System;
    using System.Runtime.InteropServices;

    struct hb_coverart_s
    {
       
        [MarshalAs(UnmanagedType.LPStr)]
        public string name; //  // char*
        public IntPtr data; // uint8_t* 
        public UInt32 size; // uint32_t
        public int type; // HB_ART_UNDEFINED, HB_ART_BMP, HB_ART_GIF, HB_ART_PNG, HB_ART_JPEG
    };

}
