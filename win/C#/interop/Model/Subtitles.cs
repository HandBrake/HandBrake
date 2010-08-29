using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace HandBrake.Interop
{
    public class Subtitles
    {
        public List<SrtSubtitle> SrtSubtitles { get; set; }
        public List<SourceSubtitle> SourceSubtitles { get; set; }
    }
}
