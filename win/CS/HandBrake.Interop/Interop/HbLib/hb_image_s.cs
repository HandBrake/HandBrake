﻿// --------------------------------------------------------------------------------------------------------------------
// <copyright file="hb_image_s.cs" company="HandBrake Project (https://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the hb_image_s type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

// ReSharper disable InconsistentNaming
// ReSharper disable UnusedMember.Global
namespace HandBrake.Interop.Interop.HbLib
{
    using System;
    using System.Runtime.InteropServices;

    /// <summary>
    /// The hb_image_s.
    /// </summary>
    internal struct hb_image_s
    {
#pragma warning disable 0649
        public int format;
        public int max_plane;
        public int width;
        public int height;
        public int color_prim;
        public int color_transfer;
        public int color_matrix;
        public IntPtr data;

        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 4, ArraySubType = UnmanagedType.I4)]
        public image_plane[] plane;
#pragma warning restore 0649
    }

    /// <summary>
    /// The image_plane.
    /// </summary>
    internal struct image_plane
    {
#pragma warning disable 0649
        public IntPtr data;
        public int width;
        public int height;
        public int stride;
        public int size;
#pragma warning restore 0649
    }
}


