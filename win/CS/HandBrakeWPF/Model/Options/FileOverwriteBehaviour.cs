// --------------------------------------------------------------------------------------------------------------------
// <copyright file="FileOverwriteBehaviour.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   File Overwrite Behaviour options
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Model.Options
{
    using HandBrake.Interop.Attributes;

    using HandBrakeWPF.Properties;

    public enum FileOverwriteBehaviour
    {
        [DisplayName(typeof(Resources), "FileOverwriteBehaviours_Ask")]
        Ask = 0,
        [DisplayName(typeof(Resources), "FileOverwriteBehaviours_Overwrite")]
        ForceOverwrite = 1,
       // [DisplayName(typeof(Resources), "FileOverwriteBehaviours_Autoname")]
        //Autoname = 2,
    }
}
