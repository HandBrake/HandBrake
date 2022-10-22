// --------------------------------------------------------------------------------------------------------------------
// <copyright file="WindowManager.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Wrapper class to decouple us from 3rd party library
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Services
{
    using System.Linq;
    using System.Windows;

    using HandBrakeWPF.Services.Interfaces;
    using HandBrakeWPF.ViewModels.Interfaces;

    public class WindowManager : IWindowManager
    {
        public bool? ShowDialog<W>(object viewModel) where W : Window, new()
        {
            Window newWindow = new W();
            newWindow.DataContext = viewModel;

            IViewModelBase viewModelBase = viewModel as IViewModelBase;
            if (viewModelBase != null)
            {
                viewModelBase.Activate();
            }

            newWindow.Closing += NewWindow_Closing;
            return newWindow.ShowDialog();
        }

        public void ShowWindow<W>(object viewModelInstance) where W : Window, new()
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
                Window newWindow = new W();
                newWindow.DataContext = viewModelInstance;

                IViewModelBase viewModelBase = viewModelInstance as IViewModelBase;
                if (viewModelBase != null)
                {
                    viewModelBase.Activate();
                }

                newWindow.Show();
                newWindow.Closing += NewWindow_Closing;
            }
        }

        private static void NewWindow_Closing(object sender, System.ComponentModel.CancelEventArgs e)
        {
            // Make sure we call deactivate on the view model to allow it to cleanup resources.
            Window window = sender as Window;
            if (window != null)
            {
                window.Closing -= NewWindow_Closing;
                IViewModelBase viewModelBase = window.DataContext as IViewModelBase;
                if (viewModelBase != null)
                {
                    viewModelBase.Deactivate();
                }
            }
        }
    }
}
