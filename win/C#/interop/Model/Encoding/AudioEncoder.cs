// --------------------------------------------------------------------------------------------------------------------
// <copyright file="AudioEncoder.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the AudioEncoder type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------
namespace HandBrake.Interop.Model.Encoding
{
    public enum AudioEncoder
    {
        [DisplayString("AAC (faac)")]
        Faac = 0,

        [DisplayString("MP3 (lame)")]
        Lame,

        [DisplayString("AC3 Passthrough")]
        Ac3Passthrough,

        [DisplayString("DTS Passthrough")]
        DtsPassthrough,

        [DisplayString("Vorbis (vorbis)")]
        Vorbis
    }
}
