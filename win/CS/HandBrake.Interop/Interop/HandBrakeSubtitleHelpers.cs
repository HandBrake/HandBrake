// --------------------------------------------------------------------------------------------------------------------
// <copyright file="HandBrakeSubtitleHelpers.cs" company="HandBrake Project (https://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the HandBrakeSubtitleHelpers type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Interop
{
    using HandBrake.Interop.Interop.HbLib;

    public class HandBrakeSubtitleHelpers
    {
        public static int CheckCanForceSubtitle(int source)
        {
            return HBFunctions.hb_subtitle_can_force(source);
        }

        public static int CheckCanBurnSubtitle(int source)
        {
            return HBFunctions.hb_subtitle_can_burn(source);
        }
    }
}
