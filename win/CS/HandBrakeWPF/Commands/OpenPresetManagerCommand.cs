using System;

namespace HandBrakeWPF.Commands
{
    using System.Windows.Input;

    using Caliburn.Micro;

    using HandBrakeWPF.ViewModels.Interfaces;

    public class OpenPresetManagerCommand : ICommand
    {
        public bool CanExecute(object? parameter)
        {
            return true;
        }

        public void Execute(object? parameter)
        {
            IMainViewModel viewModel = IoC.Get<IMainViewModel>();
            viewModel.OpenPresetWindow();
        }

        public event EventHandler? CanExecuteChanged;
    }
}
