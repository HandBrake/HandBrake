using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace HandBrake.Interop
{
    public class AudioEncoding
    {
        public int InputNumber { get; set; }
        public AudioEncoder Encoder { get; set; }
        public int Bitrate { get; set; }
        public Mixdown Mixdown { get; set; }
        public string SampleRate { get; set; }
        public double Drc { get; set; }
    }
}
