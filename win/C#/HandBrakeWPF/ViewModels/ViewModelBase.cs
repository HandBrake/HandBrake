namespace HandBrakeWPF.ViewModels
{
    using Caliburn.PresentationFramework.ApplicationModel;

    using Microsoft.Practices.ServiceLocation;

    /// <summary>
    /// A Base Class for the View Models which contains reusable code.
    /// </summary>
    public class ViewModelBase : MultiPresenterManager
    {
        protected IServiceLocator Locator { get; private set; }

        public ViewModelBase(IServiceLocator locator)
        {
            this.Locator = locator;
        }

        public void Show<T>() where T : IPresenter
        {
            this.ShutdownCurrent();
            this.Open(Locator.GetInstance<T>());
        }

        public void ShowDialog<T>() where T : IPresenter
        {
            Locator.GetInstance<IWindowManager>().ShowDialog(Locator.GetInstance<T>());
        }
        
        public void Popup<T>() where T : IPresenter
        {
            Locator.GetInstance<IWindowManager>().Show(Locator.GetInstance<T>());
        }
    }
}
