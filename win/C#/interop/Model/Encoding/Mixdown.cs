using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace HandBrake.Interop
{
    public enum Mixdown
    {
        [DisplayString("Dolby Pro Logic II")]
        DolbyProLogicII = 0,

        [DisplayString("Mono")]
        Mono,

        [DisplayString("Stereo")]
        Stereo,

        [DisplayString("Dolby Surround")]
        DolbySurround,

        [DisplayString("6 Channel Discrete")]
        SixChannelDiscrete
    }
}
