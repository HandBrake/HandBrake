// --------------------------------------------------------------------------------------------------------------------
// <copyright file="ThemeLoader.cs" company="HandBrake Project (https://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Themes
{
    using System;
    using System.Windows;
    using System.Windows.Media;

    using HandBrakeWPF.Services.Interfaces;
    using HandBrake.App.Core.Utilities;
    using HandBrakeWPF.Model;
    using HandBrakeWPF.Utilities;

    public class ThemeLoader
    {
        public static decimal ThemeOpacity { get; private set; }

        public static void LoadAppTheme(IUserSettingService userSettingService)
        {
            // App Theme
            AppThemeMode useAppTheme = (AppThemeMode)userSettingService.GetUserSetting<int>(UserSettingConstants.DarkThemeMode);

            // Set Opacity Default
            ThemeOpacity = 0.75m;

            Application.Current.Resources.MergedDictionaries.Add(new ResourceDictionary { Source = new Uri("Themes/Generic.xaml", UriKind.Relative) });
            bool themed = false;
            bool loadBaseStyle = true;
            if (SystemParameters.HighContrast || !Portable.IsThemeEnabled())
            {
                Application.Current.Resources["Ui.Light"] = new SolidColorBrush(SystemColors.HighlightTextColor);
                Application.Current.Resources["Ui.ContrastLight"] = new SolidColorBrush(SystemColors.ActiveBorderBrush.Color);
                useAppTheme = AppThemeMode.None;
            }

            switch (useAppTheme)
            {
                case AppThemeMode.System:
                    if (SystemInfo.IsAppsUsingDarkTheme())
                    {
                        Application.Current.Resources.MergedDictionaries.Add(new ResourceDictionary { Source = new Uri("Themes/Dark.xaml", UriKind.Relative) });
                    }
                    else
                    {
                        Application.Current.Resources.MergedDictionaries.Add(new ResourceDictionary { Source = new Uri("Themes/Light.xaml", UriKind.Relative) });
                    }

                    themed = true;
                    break;
                case AppThemeMode.Dark:
                    Application.Current.Resources.MergedDictionaries.Add(new ResourceDictionary { Source = new Uri("Themes/Dark.xaml", UriKind.Relative) });
                    themed = true;
                    break;
                case AppThemeMode.Light:
                    Application.Current.Resources.MergedDictionaries.Add(new ResourceDictionary { Source = new Uri("Themes/Light.xaml", UriKind.Relative) });
                    themed = true;
                    break;

                case AppThemeMode.None:
                    Application.Current.Resources["Ui.Light"] = new SolidColorBrush(SystemColors.HighlightTextColor);
                    Application.Current.Resources["Ui.ContrastLight"] = new SolidColorBrush(SystemColors.ActiveBorderBrush.Color);
                    Application.Current.Resources["Ui.Background"] = new SolidColorBrush(SystemColors.WindowColor);
                    Application.Current.Resources["Ui.BackgroundDark"] = new SolidColorBrush(SystemColors.WindowTextColor);
                    themed = false;
                    break;

                case AppThemeMode.Modern:
                    // This theme is not ready for use.
                    ThemeOpacity = 1m;
                    Application.Current.Resources.MergedDictionaries.Add(new ResourceDictionary { Source = new Uri("pack://application:,,,/PresentationFramework.Fluent;component/Themes/Fluent.xaml", UriKind.RelativeOrAbsolute) });
                    loadBaseStyle = false;
                    break;
            }

            Application.Current.Resources.MergedDictionaries.Add(new ResourceDictionary { Source = new Uri("Views/Styles/Styles.xaml", UriKind.Relative) });
            if (loadBaseStyle)
            {
                Application.Current.Resources.MergedDictionaries.Add(new ResourceDictionary { Source = new Uri("Views/Styles/BaseStyles.xaml", UriKind.Relative) });
            }

            if (themed)
            {
                Application.Current.Resources.MergedDictionaries.Add(new ResourceDictionary { Source = new Uri("Views/Styles/ThemedStyles.xaml", UriKind.Relative) });
            }
        }
    }
}
