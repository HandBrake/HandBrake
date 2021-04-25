// --------------------------------------------------------------------------------------------------------------------
// <copyright file="FlipRotationCommand.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the FlipRotationCommand type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Model.Picture
{
    public class FlipRotationCommand
    {
        public FlipRotationCommand(ChangedPictureField changedField, int? previousRotation, int? previousHflip)
        {
            this.ChangedField = changedField;
            this.PreviousRotation = previousRotation;
            this.PreviousHflip = previousHflip;
        }

        public ChangedPictureField ChangedField { get; }

        public int? PreviousRotation { get; }

        public int? PreviousHflip { get; }
    }
}
