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
    using System.Globalization;
    using System.Linq;
    using System.Windows;
    using System.Windows.Controls;
    using System.Windows.Data;

    using HandBrakeWPF.Commands;
    using HandBrakeWPF.Commands.Presets;
    using HandBrakeWPF.Helpers;
    using HandBrakeWPF.Model.Options;
    using HandBrakeWPF.Properties;
    using HandBrakeWPF.Services.Interfaces;
    using HandBrakeWPF.Services.Presets;
    using HandBrakeWPF.Services.Presets.Interfaces;
    using HandBrakeWPF.Services.Presets.Model;

    /// <summary>
    /// The presets menu converter.
    /// </summary>
    public class PresetsMenuConverter : IValueConverter
    {
        private readonly IUserSettingService userSettingService;
        
        public PresetsMenuConverter()
        {
            this.userSettingService = IoCHelper.Get<IUserSettingService>();
        }

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
            
            List<object> groupedMenu = new List<object>();

            // Generate the Preset Manager Item
            if (parameter != null && "true".Equals(parameter))
            {
                MenuItem savePresetMenuItem = new MenuItem
                                                 {
                                                     Header = Resources.MainView_SaveNewPreset,
                                                     Tag = null,
                                                     Command = new AddPresetCommand()
                                                 };
                groupedMenu.Add(savePresetMenuItem);

                MenuItem presetManagerMenuItem = new MenuItem
                                        {
                                            Header = Resources.PresetManger_Title,
                                            Tag = null,
                                            Command = new OpenPresetManagerCommand()
                                        };
                groupedMenu.Add(presetManagerMenuItem);


                groupedMenu.Add(new Separator());
            }

            IEnumerable<IPresetObject> presetObjects = presets.ToList();

            PresetDisplayMode mode = userSettingService.GetUserSetting<PresetDisplayMode>(UserSettingConstants.PresetMenuDisplayMode);

            switch (mode)
            {
                case PresetDisplayMode.Flat:
                    GenerateFlatList(groupedMenu, presetObjects.ToList());
                    break;

                case PresetDisplayMode.Partial:
                    GenerateUserPresets(groupedMenu, presetObjects.FirstOrDefault(p => p.Category == PresetService.UserPresetCategoryName));
                    GeneratePresets(groupedMenu, presetObjects.ToList(), mode);
                    break;

                case PresetDisplayMode.Category:
                    GeneratePresets(groupedMenu, presetObjects.ToList(), mode);
                    break;
            }

            return groupedMenu;
        }
        
        private void GenerateUserPresets(List<object> groupedMenu, IPresetObject userPresets)
        {
            PresetDisplayCategory category = userPresets as PresetDisplayCategory;
            if (category != null)
            {
                if (category.Presets.Count != 0)
                {
                    groupedMenu.Add(GenerateMenuHeader(category.Category));
                    groupedMenu.Add(new Separator());
                }

                foreach (var preset in category.Presets.Reverse())
                {
                    groupedMenu.Add(GeneratePresetMenuItem(preset));
                }
            }
        }

        private void GeneratePresets(List<object> groupedMenu, IList<IPresetObject> userPresets, PresetDisplayMode displayMode)
        {
            if (!userPresets.Any(s => s.Category != PresetService.UserPresetCategoryName))
            {
                return;
            }

            if (displayMode == PresetDisplayMode.Partial)
            {
                groupedMenu.Add(GenerateMenuHeader(Resources.Main_Presets));
                groupedMenu.Add(new Separator());
            }

            foreach (IPresetObject presetCategory in userPresets)
            {
                if (displayMode == PresetDisplayMode.Partial && PresetService.UserPresetCategoryName.Equals(presetCategory.Category))
                {
                    continue; // In Partially Flat mode, we show the user presets flat so no need to include the category. 
                }

                PresetDisplayCategory category = presetCategory as PresetDisplayCategory;
                if (category != null)
                {
                    groupedMenu.Add(GeneratePresetGroup(category));
                }
            }
        }

        private MenuItem GeneratePresetGroup(PresetDisplayCategory category)
        {
            MenuItem group = new MenuItem();
            group.Header = category.Category;
            foreach (var preset in category.Presets)
            {
                group.Items.Add(GeneratePresetMenuItem(preset));
            }

            return group;
        }

        private void GenerateFlatList(List<object> groupedMenu, IList<IPresetObject> userPresets)
        {
            foreach (IPresetObject presetCategory in userPresets)
            {
                PresetDisplayCategory category = presetCategory as PresetDisplayCategory;
                if (category != null)
                {
                    groupedMenu.Add(GenerateMenuHeader(category.Category));

                    if (groupedMenu.Count != 0 && groupedMenu.LastOrDefault()?.GetType() != typeof(Separator))
                    {
                        groupedMenu.Add(new Separator());
                    }
                    
                    foreach (var preset in category.Presets)
                    {
                        groupedMenu.Add(GeneratePresetMenuItem(preset));
                    }
                }
            }
        }

        private MenuItem GeneratePresetMenuItem(Preset preset)
        {
            MenuItem newMenuItem = new MenuItem { Header = preset.Name, Tag = preset, Command = new PresetMenuSelectCommand(preset), IsEnabled = !preset.IsPresetDisabled, ToolTip = preset.Description };

            if (preset.IsDefault)
            {
                newMenuItem.Header = string.Format("{0} {1}", preset.Name, Resources.Preset_Default);
                newMenuItem.FontStyle = FontStyles.Italic;
            }

            if (preset.IsPresetDisabled)
            {
                newMenuItem.Header = string.Format("{0} {1}", preset.Name, Resources.Preset_NotAvailable);
            }
            
            return newMenuItem;
        }

        private MenuItem GenerateMenuHeader(string name)
        {
            MenuItem presetLabelMenuItem = new MenuItem
                                           {
                                               Header = name,
                                               Tag = null,
                                               IsEnabled = false,
                                               FontSize = 12.5,
                                               Margin = new Thickness(0, 2, 0, 2)
                                           };

            presetLabelMenuItem.FontWeight = FontWeights.Bold;
            presetLabelMenuItem.FontStyle = FontStyles.Normal;
            presetLabelMenuItem.Margin = new Thickness(0, 4, 0, 0);

            return presetLabelMenuItem;
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }
}
