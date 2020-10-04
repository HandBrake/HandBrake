// --------------------------------------------------------------------------------------------------------------------
// <copyright file="HandBrakePictureHelpers.cs" company="HandBrake Project (https://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the HandBrakePictureHelpers type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Interop
{
    using HandBrake.Interop.Interop.HbLib;

    public class HandBrakePictureHelpers
    {
        public static hb_geometry_s GetAnamorphicSizes(hb_geometry_s sourceGeometry, hb_geometry_settings_s uiGeometry)
        {
            hb_geometry_s geometry = new hb_geometry_s();

            HBFunctions.hb_set_anamorphic_size2(ref sourceGeometry, ref uiGeometry, ref geometry);

            return geometry;
        }
    }
}
