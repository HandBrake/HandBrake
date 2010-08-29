using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace HandBrake.Interop
{
    public enum VideoEncoder
    {
        [DisplayString("H.264 (x264)")]
        X264 = 0,

        [DisplayString("MPEG-4 (FFMpeg)")]
        FFMpeg,

        [DisplayString("VP3 (Theora)")]
        Theora
    }
}
