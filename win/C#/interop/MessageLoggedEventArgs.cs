using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace HandBrake.Interop
{
    public class MessageLoggedEventArgs : EventArgs
    {
        public string Message { get; set; }
    }
}
