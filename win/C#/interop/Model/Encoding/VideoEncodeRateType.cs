using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace HandBrake.Interop
{
    public enum VideoEncodeRateType
    {
        TargetSize = 0,
        AverageBitrate,
        ConstantQuality
    }
}
