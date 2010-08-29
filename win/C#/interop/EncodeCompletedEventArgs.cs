using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace HandBrake.Interop
{
    public class EncodeCompletedEventArgs : EventArgs
    {
        public bool Error { get; set; }
    }
}
