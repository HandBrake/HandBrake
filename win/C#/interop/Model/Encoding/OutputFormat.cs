using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.ComponentModel;

namespace HandBrake.Interop
{
    public enum OutputFormat
    {
        [DisplayString("MP4")]
        Mp4,
        [DisplayString("MKV")]
        Mkv
    }
}
