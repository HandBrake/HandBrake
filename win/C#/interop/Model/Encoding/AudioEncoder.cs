using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace HandBrake.Interop
{
    public enum AudioEncoder
    {
        [DisplayString("AAC (faac)")]
        Faac = 0,

        [DisplayString("MP3 (lame)")]
        Lame,

        [DisplayString("AC3 Passthrough")]
        Ac3Passthrough,

        [DisplayString("DTS Passthrough")]
        DtsPassthrough,

        [DisplayString("Vorbis (vorbis)")]
        Vorbis
    }
}
