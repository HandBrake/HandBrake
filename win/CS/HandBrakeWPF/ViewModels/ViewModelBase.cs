namespace HandBrakeWPF.ViewModels
{
    using Caliburn.Micro;

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
