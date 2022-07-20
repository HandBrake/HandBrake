// --------------------------------------------------------------------------------------------------------------------
// <copyright file="WindowHelper.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Generic Window Helper to allow spawning or re-opening windows.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Helpers
{
    using System;
    using System.Linq;
    using System.Windows;


    using HandBrakeWPF.Services.Interfaces;
    using HandBrakeWPF.ViewModels.Interfaces;

    public class WindowHelper
    {
        public static void ShowWindow<T, W>(IWindowManager windowManager) where W : Window, new()
        {
            Window window = Application.Current.Windows.Cast<Window>().FirstOrDefault(x => x.GetType() == typeof(W));

            if (window != null)
            {
                if (window.WindowState == WindowState.Minimized)
                {
                    window.WindowState = WindowState.Normal;
                }
                window.Activate();
            }
            else
            {
                IViewModelBase viewModel = IoCHelper.Get<T>() as IViewModelBase;
                windowManager.ShowWindow<W>(viewModel);
            }
        }

        public static void CloseWindow(bool? dialogResult, string viewName)
        {
            Type windowType = Type.GetType("HandBrakeWPF.Views." + viewName);

            Window window = Application.Current.Windows.Cast<Window>().FirstOrDefault(x => x.GetType() == windowType);

            if (window != null)
            {
                if (dialogResult != null)
                {
                    window.DialogResult = dialogResult;
                }

                window.Close();
            }
        }
    }
}
