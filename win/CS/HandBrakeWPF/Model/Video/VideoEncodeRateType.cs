// --------------------------------------------------------------------------------------------------------------------
// <copyright file="VideoEncodeRateType.cs" company="HandBrake Project (https://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the VideoEncodeRateType type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Model.Video
{
    /// <summary>
    /// The video encode rate type.
    /// </summary>
    public enum VideoEncodeRateType
    {
        TargetSize = 0,
        AverageBitrate = 1,
        ConstantQuality = 2
    }
}
