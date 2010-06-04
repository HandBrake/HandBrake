using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace HandBrake.Interop
{
    public enum Anamorphic
    {
        [DisplayString("None")]
        None = 0,
        [DisplayString("Strict")]
        Strict,
        [DisplayString("Loose")]
        Loose,
        [DisplayString("Custom")]
        Custom
    }
}
