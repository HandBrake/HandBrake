namespace HandBrake.Interop.HbLib
{
    using System;
    using System.Runtime.InteropServices;

    public struct hb_image_s
    {
        public int format;
        public int width;
        public int height;
        public IntPtr data;

        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 4, ArraySubType = UnmanagedType.I4)]
        public image_plane[] plane;
    }

    public struct image_plane
    {
        public IntPtr data;
        public int width;
        public int height;
        public int stride;
        public int height_stride;
        public int size;
    }
}
