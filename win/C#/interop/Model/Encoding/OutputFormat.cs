// --------------------------------------------------------------------------------------------------------------------
// <copyright file="OutputFormat.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the OutputFormat type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Model.Encoding
{
    /// <summary>
    /// Output File Format
    /// </summary>
    public enum OutputFormat
    {
        [DisplayString("MP4")]
        Mp4,
        [DisplayString("MKV")]
        Mkv
    }
}
