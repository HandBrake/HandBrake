﻿// --------------------------------------------------------------------------------------------------------------------
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
    using System.Diagnostics;
    using System.Globalization;
    using System.Linq;
    using System.Windows;
    using System.Windows.Controls;
    using System.Windows.Data;
    using HandBrakeWPF.Commands;
    using HandBrakeWPF.Properties;
    using HandBrakeWPF.Services.Presets.Interfaces;
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
            IEnumerable<IPresetObject> presets = value as IEnumerable<IPresetObject>;

            if (presets == null)
            {
                return null;
            }
            
            Dictionary<string, object> groupedMenu = new Dictionary<string, object>();

            if (parameter != null && "true".Equals(parameter))
            {
                MenuItem presetManagerMenuItem = new MenuItem
                                        {
                                            Header = Resources.PresetManger_Title,
                                            Tag = null,
                                            Command = new OpenPresetManagerCommand()
                                        };
                groupedMenu.Add("hb_preset_manager", presetManagerMenuItem);
                groupedMenu.Add("hb_menu_separator", new Separator());
            }

            foreach (IPresetObject item in presets)
            {
                PresetDisplayCategory category = item as PresetDisplayCategory;
                if (category != null)
                {
                    ProcessCategory(groupedMenu, category);
                    continue;
                }

                Preset preset = item as Preset;
                if (preset != null)
                {
                    ProcessPreset(groupedMenu, preset);
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

        private void ProcessPreset(Dictionary<string, object> groupedMenu, Preset preset)
        {
            if (groupedMenu.ContainsKey(preset.Category))
            {
                MenuItem newMenuItem = new MenuItem { Header = preset.Name, Tag = preset, Command = new PresetMenuSelectCommand(preset), IsEnabled = !preset.IsPresetDisabled, ToolTip = preset.Description };

                if (preset.IsPresetDisabled)
                {
                    newMenuItem.Header = string.Format("{0} {1}", preset.Name, Resources.Preset_NotAvailable);
                }

                if (preset.IsDefault)
                {
                    newMenuItem.FontStyle = FontStyles.Italic;
                    newMenuItem.FontSize = 14;
                }

                MenuItem menu = groupedMenu[preset.Category] as MenuItem;
                menu?.Items.Add(newMenuItem);
            }
            else
            {
                MenuItem group = new MenuItem();
                group.Header = preset.Category;

                MenuItem newMenuItem = new MenuItem { Header = preset.Name, Tag = preset, Command = new PresetMenuSelectCommand(preset), IsEnabled = !preset.IsPresetDisabled, ToolTip = preset.Description };

                if (preset.IsPresetDisabled)
                {
                    newMenuItem.Header = string.Format("{0} {1}", preset.Name, Resources.Preset_NotAvailable);
                }

                if (preset.IsDefault)
                {
                    newMenuItem.FontStyle = FontStyles.Italic;
                    newMenuItem.FontSize = 14;
                }

                group.Items.Add(newMenuItem);
                groupedMenu[preset.Category] = group;
            }
        }

        private void ProcessCategory(Dictionary<string, object> groupedMenu, PresetDisplayCategory category)
        {
            foreach (Preset preset in category.Presets)
            {
                this.ProcessPreset(groupedMenu, preset);
            }
        }
    }
}
