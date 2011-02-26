namespace HandBrakeWPF.Services
{
    using System;
    using System.Windows;

    using Caliburn.PresentationFramework.ApplicationModel;

    public class WindowManager : DefaultWindowManager, IWindowManager
    {

        public WindowManager(IViewStrategy viewStrategy, IBinder binder)

            : base(viewStrategy, binder)
        {
        }

        //Display a view in a dialog (modal) window 
        public new bool? ShowDialog(object rootModel, object context, Action<ISubordinate, Action> handleShutdownModel)
        {
            var window = base.CreateWindow(rootModel, true, context, handleShutdownModel);
            window.WindowStartupLocation = WindowStartupLocation.CenterScreen;
            window.WindowStyle = WindowStyle.ToolWindow;
            window.ResizeMode = ResizeMode.NoResize;
            window.Title = ((IPresenter)rootModel).DisplayName;
            return window.ShowDialog();
        }

        //Display a view in a popup (non-modal) window 
        public new void Show(object rootModel, object context, Action<ISubordinate, Action> handleShutdownModel)
        {
            var window = base.CreateWindow(rootModel, false, context, handleShutdownModel);
            window.WindowStartupLocation = WindowStartupLocation.CenterScreen;
            window.Title = ((IPresenter)rootModel).DisplayName;
            window.ResizeMode = ResizeMode.NoResize;
            window.Show();
        }

    }

}
