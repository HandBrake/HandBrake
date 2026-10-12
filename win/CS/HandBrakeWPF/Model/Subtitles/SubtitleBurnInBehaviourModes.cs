// --------------------------------------------------------------------------------------------------------------------
// <copyright file="BurnPassthruModes.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Model.Subtitles
{
    using HandBrake.Interop.Attributes;

    using HandBrakeWPF.Properties;

    public enum SubtitleBurnInBehaviourModes
    {
        [DisplayName(typeof(Resources), "BurnPassthruModes_PassthruDrop")]
        [ShortName("PassthruDrop")]
        PassthruDrop = 0,

        [DisplayName(typeof(Resources), "BurnPassthruModes_PassthruBurn")]
        [ShortName("PassthruBurnDrop")]
        PassthruBurnDrop,

        [DisplayName(typeof(Resources), "BurnPassthruModes_Burn")]
        [ShortName("Burn")]
        BurnDrop,

        // TODO, Not supported in libhb yet.
        //[DisplayName(typeof(Resources), "BurnPassthruModes_Export")]
        //[ShortName("Export")]
        //Export,
    }
}
