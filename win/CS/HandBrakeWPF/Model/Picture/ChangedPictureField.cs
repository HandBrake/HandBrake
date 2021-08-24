// --------------------------------------------------------------------------------------------------------------------
// <copyright file="ChangedPictureField.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the ChangedPictureField type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Model.Picture
{
    public enum ChangedPictureField
    {
        Width,
        Height,
        ParW,
        ParH,
        DisplayWidth,
        Crop,
        Anamorphic,
        MaintainAspectRatio,
        Flip,
        Rotate,
        AllowUpscale,
        ResolutionLimit,
        OptimalSize,
        Padding
    }
}
