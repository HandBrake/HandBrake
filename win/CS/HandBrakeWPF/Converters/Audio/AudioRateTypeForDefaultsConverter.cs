// --------------------------------------------------------------------------------------------------------------------
// <copyright file="AudioRateTypeConverter.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The audio rate type converter.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Converters.Audio
{
    using System;
    using System.Collections.Generic;
    using System.Globalization;
    using System.Linq;
    using System.Windows.Data;

    using HandBrake.Interop.Interop;
    using HandBrake.Interop.Interop.Interfaces.Model.Encoders;

    using HandBrakeWPF.Services.Encode.Model.Models;
    using HandBrakeWPF.Utilities;

    using AudioEncoderRateType = Services.Encode.Model.Models.AudioEncoderRateType;

    public class AudioRateTypeForDefaultsConverter : IMultiValueConverter
    {
        public object Convert(object[] values, Type targetType, object parameter, CultureInfo culture)
        {
            if (values == null || values.Length != 2)
            {
                return null;
            }

            IList<AudioEncoderRateType> fetchRateTypes = EnumHelper<AudioEncoderRateType>.GetEnumList().ToList();
            List<string> types = new List<string>();
            foreach (var item in fetchRateTypes)
            {
                types.Add(EnumHelper<AudioEncoderRateType>.GetDisplay(item));
            }


            AudioEncoder audioEncoder = values[0] is AudioEncoder ? (AudioEncoder)values[0] : AudioEncoder.None;
            AudioEncoder fallbackEncoder = values[1] is AudioEncoder ? (AudioEncoder)values[1] : AudioEncoder.None;
            
            HBAudioEncoder selectedEncoder = HandBrakeEncoderHelpers.GetAudioEncoder(EnumHelper<AudioEncoder>.GetShortName(audioEncoder));
            HBAudioEncoder selectedFallbackEncoder = HandBrakeEncoderHelpers.GetAudioEncoder(EnumHelper<AudioEncoder>.GetShortName(fallbackEncoder));

            if (selectedEncoder != null && selectedEncoder.IsPassthrough)
            {
                if (selectedFallbackEncoder != null && !selectedFallbackEncoder.SupportsQuality)
                {
                    types.Remove(EnumHelper<AudioEncoderRateType>.GetDisplay(AudioEncoderRateType.Quality));
                }
            }

            return types;
        }

        public object[] ConvertBack(object value, Type[] targetTypes, object parameter, CultureInfo culture)
        {
            return null;
        }
    }
}
