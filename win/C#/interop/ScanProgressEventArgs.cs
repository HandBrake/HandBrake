using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace HandBrake.Interop
{
    public class ScanProgressEventArgs : EventArgs
    {
        public int CurrentTitle { get; set; }
        public int Titles { get; set; }
    }
}
