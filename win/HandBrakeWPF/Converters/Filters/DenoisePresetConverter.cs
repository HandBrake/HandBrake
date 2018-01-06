// --------------------------------------------------------------------------------------------------------------------
// <copyright file="DenoisePresetConverter.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the DenoisePresetConverter type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Converters.Filters
{
    using System;
    using System.Collections.Generic;
    using System.Globalization;
    using System.Linq;
    using System.Windows.Data;

    using HandBrake.ApplicationServices.Interop.Model.Encoding;

    using DenoisePreset = HandBrakeWPF.Services.Encode.Model.Models.DenoisePreset;

    /// <summary>
    /// The denoise preset converter.
    /// </summary>
    public class DenoisePresetConverter : IMultiValueConverter
    {
        /// <summary>
        /// The convert.
        /// </summary>
        /// <param name="values">
        /// The values.
        /// </param>
        /// <param name="targetType">
        /// The target type.
        /// </param>
        /// <param name="parameter">
        /// The parameter.
        /// </param>
        /// <param name="culture">
        /// The culture.
        /// </param>
        /// <returns>
        /// The <see cref="object"/>.
        /// </returns>
        public object Convert(object[] values, Type targetType, object parameter, CultureInfo culture)
        {
            if (values.Any() && values.Count() == 2)
            {
                Denoise denoiseChoice = (Denoise)values[1];

                if (denoiseChoice == Denoise.hqdn3d)
                {
                    return new List<DenoisePreset> { DenoisePreset.Weak, DenoisePreset.Medium, DenoisePreset.Strong, DenoisePreset.Custom };
                }

                if (denoiseChoice == Denoise.NLMeans)
                {
                    return new List<DenoisePreset> { DenoisePreset.Ultralight, DenoisePreset.Light, DenoisePreset.Medium, DenoisePreset.Strong, DenoisePreset.Custom };
                } 
            }

            return Enumerable.Empty<DenoisePreset>();
        }

        /// <summary>
        /// The convert back. Not used
        /// </summary>
        /// <param name="value">
        /// The value.
        /// </param>
        /// <param name="targetTypes">
        /// The target types.
        /// </param>
        /// <param name="parameter">
        /// The parameter.
        /// </param>
        /// <param name="culture">
        /// The culture.
        /// </param>
        /// <returns>
        /// The Nothing. Not used
        /// </returns>
        public object[] ConvertBack(object value, Type[] targetTypes, object parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }
}
