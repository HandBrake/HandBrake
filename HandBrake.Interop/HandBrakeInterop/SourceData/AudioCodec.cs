// --------------------------------------------------------------------------------------------------------------------
// <copyright file="AudioCodec.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the AudioCodec type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.SourceData
{
    /// <summary>
    /// The audio codec.
    /// Only contains 2 real codecs at the moment as those are what we care about. More will be added later.
    /// </summary>
    public enum AudioCodec
    {
        /// <summary>
        /// The ac 3.
        /// </summary>
        Ac3, 

        /// <summary>
        /// The dts.
        /// </summary>
        Dts, 

        /// <summary>
        /// The dts hd.
        /// </summary>
        DtsHD, 

        /// <summary>
        /// The mp 3.
        /// </summary>
        Mp3, 

        /// <summary>
        /// The aac.
        /// </summary>
        Aac, 

        /// <summary>
        /// The other.
        /// </summary>
        Other, 

        /// <summary>
        /// The flac.
        /// </summary>
        Flac
    }
}