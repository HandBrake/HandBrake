// --------------------------------------------------------------------------------------------------------------------
// <copyright file="PresetsMenuConverter.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   A Converter to manage the Presets list and turn it into a grouped set of MenuItems
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Converters
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Globalization;
    using System.Windows;
    using System.Windows.Controls;
    using System.Windows.Data;

    using HandBrakeWPF.Commands;
    using HandBrakeWPF.Services.Presets.Model;

    /// <summary>
    /// The presets menu converter.
    /// </summary>
    public class PresetsMenuConverter : IValueConverter
    {
        /// <summary>Converts a value. </summary>
        /// <returns>A converted value. If the method returns null, the valid null value is used.</returns>
        /// <param name="value">The value produced by the binding source.</param>
        /// <param name="targetType">The type of the binding target property.</param>
        /// <param name="parameter">The converter parameter to use.</param>
        /// <param name="culture">The culture to use in the converter.</param>
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            IEnumerable<Preset> presets = value as IEnumerable<Preset>;

            if (presets == null)
            {
                return null;
            }
            
            Dictionary<string, MenuItem> groupedMenu = new Dictionary<string, MenuItem>();
            foreach (Preset item in presets)
            {
                if (groupedMenu.ContainsKey(item.Category))
                {
                    MenuItem newMeuItem = new MenuItem { Header = item.Name, Tag = item, Command = new PresetMenuSelectCommand(item)};
                    if (item.IsDefault)
                    {
                        newMeuItem.FontStyle = FontStyles.Italic;
                    }

                    groupedMenu[item.Category].Items.Add(newMeuItem);
                }
                else
                {
                    MenuItem group = new MenuItem();
                    group.Header = item.Category;

                    MenuItem newMeuItem = new MenuItem { Header = item.Name, Tag = item, Command = new PresetMenuSelectCommand(item) };
                    if (item.IsDefault)
                    {
                        newMeuItem.FontStyle = FontStyles.Italic;
                    }

                    group.Items.Add(newMeuItem);
                    groupedMenu[item.Category] = group;
                }
            }

            return groupedMenu.Values.ToList();
        }

        /// <summary>Converts a value. </summary>
        /// <returns>A converted value. If the method returns null, the valid null value is used.</returns>
        /// <param name="value">The value that is produced by the binding target.</param>
        /// <param name="targetType">The type to convert to.</param>
        /// <param name="parameter">The converter parameter to use.</param>
        /// <param name="culture">The culture to use in the converter.</param>
        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }
}
