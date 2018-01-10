// --------------------------------------------------------------------------------------------------------------------
// <copyright file="ViewManager.cs" company="HandBrake Project (http://handbrake.fr)">
// This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Services
{
    using System.Linq;
    using System.Windows;
    using Caliburn.Micro;
    using HandBrake.Commands;
    using HandBrake.Model;
    using HandBrake.Services.Interfaces;

    public class ViewManager : IViewManager
    {
        public IWindowManager WindowManager => windowManager ?? (windowManager = IoC.Get<IWindowManager>());

        private IWindowManager windowManager;

        public ViewManager()
        {
        }

        public bool SupportsWindow => true;

        public bool? ShowDialog<TViewModel>(TViewModel viewmodel = default(TViewModel))
        {
            if (viewmodel == null)
            {
                viewmodel = IoC.Get<TViewModel>();
            }

            return this.WindowManager.ShowDialog(viewmodel);
        }

        public void ShowWindow<TViewModel>(TViewModel viewmodel = default(TViewModel))
        {
            if (viewmodel == null)
            {
                viewmodel = IoC.Get<TViewModel>();
            }

            Window window = Application.Current.Windows
                .Cast<Window>()
                .FirstOrDefault(x => x.DataContext?.GetType() == typeof(TViewModel));

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
                this.WindowManager.ShowWindow(viewmodel);
            }
        }

        public void OpenOptions(OptionsTab tab)
        {
            OpenOptionsScreenCommand command = new OpenOptionsScreenCommand();
            command.Execute(tab);
        }
    }
}