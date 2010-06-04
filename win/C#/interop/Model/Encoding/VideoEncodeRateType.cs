// --------------------------------------------------------------------------------------------------------------------
// <copyright file="VideoEncodeRateType.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the VideoEncodeRateType type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Model.Encoding
{
    /// <summary>
    /// Video Encoder Encode Mode
    /// </summary>
    public enum VideoEncodeRateType
    {
        TargetSize = 0,
        AverageBitrate,
        ConstantQuality
    }
}
