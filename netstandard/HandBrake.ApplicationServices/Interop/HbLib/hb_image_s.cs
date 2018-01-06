// --------------------------------------------------------------------------------------------------------------------
// <copyright file="hb_image_s.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the hb_image_s type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ApplicationServices.Interop.HbLib
{
    using System;
    using System.Runtime.InteropServices;

    /// <summary>
    /// The hb_image_s.
    /// </summary>
    internal struct hb_image_s
    {
        public int format;
        public int width;
        public int height;
        public IntPtr data;

        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 4, ArraySubType = UnmanagedType.I4)]
        public image_plane[] plane;
    }

    /// <summary>
    /// The image_plane.
    /// </summary>
    internal struct image_plane
    {
        public IntPtr data;
        public int width;
        public int height;
        public int stride;
        public int height_stride;
        public int size;
    }
}
