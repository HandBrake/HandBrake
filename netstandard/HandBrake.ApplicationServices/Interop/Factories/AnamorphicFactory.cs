// --------------------------------------------------------------------------------------------------------------------
// <copyright file="AnamorphicFactory.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The Anamorphic factory.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ApplicationServices.Interop.Factories
{
    using System.Collections.Generic;

    using HandBrake.ApplicationServices.Interop.Json.Anamorphic;
    using HandBrake.ApplicationServices.Interop.Json.Shared;
    using HandBrake.ApplicationServices.Interop.Model;
    using HandBrake.ApplicationServices.Interop.Model.Encoding;
    using HandBrake.ApplicationServices.Interop.Model.Preview;

    /// <summary>
    /// The anamorphic factory.
    /// </summary>
    public class AnamorphicFactory
    {
        /// <summary>
        /// The keep setting.
        /// </summary>
        public enum KeepSetting
        {
            HB_KEEP_WIDTH = 0x01, 
            HB_KEEP_HEIGHT = 0x02, 
            HB_KEEP_DISPLAY_ASPECT = 0x04
        }

        /// <summary>
        /// Finds output geometry for the given preview settings and title.
        /// </summary>
        /// <param name="settings">
        /// The preview settings.
        /// </param>
        /// <param name="title">
        /// Information on the title to consider.
        /// </param>
        /// <returns>
        /// Geometry Information
        /// </returns>
        public static Geometry CreateGeometry(PreviewSettings settings, SourceVideoInfo title)
        {
            int settingMode = settings.KeepDisplayAspect ? 0x04 : 0;

            // Sanitize the Geometry First.
            AnamorphicGeometry anamorphicGeometry = new AnamorphicGeometry
            {
                SourceGeometry = new Geometry
                {
                    Width = title.Resolution.Width, 
                    Height = title.Resolution.Height, 
                    PAR = new PAR { Num = title.ParVal.Width, Den = title.ParVal.Height }
                }, 
                DestSettings = new DestSettings
                {
                    AnamorphicMode = (int)settings.Anamorphic, 
                    Geometry =
                    {
                        Width = settings.Width, 
                        Height = settings.Height, 
                        PAR = new PAR
                        {
                            Num = settings.Anamorphic != Anamorphic.Custom ? title.ParVal.Width : settings.PixelAspectX, 
                            Den = settings.Anamorphic != Anamorphic.Custom ? title.ParVal.Height : settings.PixelAspectY, 
                        }
                    }, 
                    Keep = settingMode, 
                    Crop = new List<int> { settings.Cropping.Top, settings.Cropping.Bottom, settings.Cropping.Left, settings.Cropping.Right }, 
                    Modulus = settings.Modulus ?? 16, 
                    MaxWidth = settings.MaxWidth, 
                    MaxHeight = settings.MaxHeight, 
                    ItuPAR = false
                }
            };

            if (settings.Anamorphic == Anamorphic.Custom)
            {
                anamorphicGeometry.DestSettings.Geometry.PAR = new PAR { Num = settings.PixelAspectX, Den = settings.PixelAspectY };
            }
            else
            {
                anamorphicGeometry.DestSettings.Geometry.PAR = new PAR { Num = title.ParVal.Width, Den = title.ParVal.Height };
            }

            return HandBrakeUtils.GetAnamorphicSize(anamorphicGeometry);
        }
    }
}
