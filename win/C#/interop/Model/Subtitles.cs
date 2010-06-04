namespace HandBrake.Interop.Model
{
    using System.Collections.Generic;

    public class Subtitles
    {
        public List<SrtSubtitle> SrtSubtitles { get; set; }
        public List<SourceSubtitle> SourceSubtitles { get; set; }
    }
}
