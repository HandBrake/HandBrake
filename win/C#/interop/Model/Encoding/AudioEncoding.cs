// --------------------------------------------------------------------------------------------------------------------
// <copyright file="AudioEncoding.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the AudioEncoding type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Model.Encoding
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
