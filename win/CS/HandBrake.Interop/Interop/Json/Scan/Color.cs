// --------------------------------------------------------------------------------------------------------------------
// <copyright file="Color.cs" company="HandBrake Project (https://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//    Colour Information 
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Interop.Json.Scan
{
    /// <summary>
    /// Colour Information 
    /// </summary>
    public class Color
    {
        public int BitDepth { get; set; }

        public int ChromaLocation { get; set; }

        public string ChromaSubsampling { get; set; }

        public int Format { get; set; }

        public int Matrix { get; set; }

        public int Primary { get; set; }

        public int Range { get; set; }

        public int Transfer { get; set; }
    }
}