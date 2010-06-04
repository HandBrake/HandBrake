using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace HandBrake.Interop
{
    public class SrtSubtitle
    {
        public bool Default { get; set; }
        public string FileName { get; set; }
        public string LanguageCode { get; set; }
        public string CharacterCode { get; set; }
        public int Offset { get; set; }

        public SrtSubtitle Clone()
        {
            return new SrtSubtitle
            {
                Default = this.Default,
                FileName = this.FileName,
                LanguageCode = this.LanguageCode,
                CharacterCode = this.CharacterCode,
                Offset = this.Offset
            };
        }
    }
}
