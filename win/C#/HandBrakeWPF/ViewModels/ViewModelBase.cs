namespace HandBrakeWPF.ViewModels
{
    using Caliburn.PresentationFramework.ApplicationModel;
    using Caliburn.PresentationFramework.Screens;

    /// <summary>
    /// A Base Class for the View Models which contains reusable code.
    /// </summary>
    public class ViewModelBase : Screen
    {
        public ViewModelBase(IWindowManager windowManager)
        {
            this.WindowManager = windowManager;
        }

        public IWindowManager WindowManager { get; private set; }
    }
}
