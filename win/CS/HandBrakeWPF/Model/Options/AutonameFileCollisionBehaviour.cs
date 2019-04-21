// --------------------------------------------------------------------------------------------------------------------
// <copyright file="AutonameFileCollisionBehaviour.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Model.Options
{
    using HandBrake.Interop.Attributes;

    using HandBrakeWPF.Properties;

    public enum AutonameFileCollisionBehaviour
    {
        [DisplayName(typeof(Resources), "CollisionBehaviour_AppendNumber")]
        AppendNumber = 0,
        [DisplayName(typeof(Resources), "CollisionBehaviour_Pre")]
        Prefix = 1,
        [DisplayName(typeof(Resources), "CollisionBehaviour_Post")]
        Postfix = 2,
    }
}
