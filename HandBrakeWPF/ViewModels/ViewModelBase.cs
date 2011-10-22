namespace HandBrakeWPF.ViewModels
{
    using Caliburn.Micro;

    /// <summary>
    /// A Base Class for the View Models which contains reusable code.
    /// </summary>
    public class ViewModelBase : Screen
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="ViewModelBase"/> class.
        /// </summary>
        /// <param name="windowManager">
        /// The window manager.
        /// </param>
        public ViewModelBase(IWindowManager windowManager)
        {
            this.WindowManager = windowManager;
        }

        /// <summary>
        /// Gets WindowManager.
        /// </summary>
        public IWindowManager WindowManager { get; private set; }
    }
}
