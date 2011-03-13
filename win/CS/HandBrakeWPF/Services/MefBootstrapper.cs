namespace HandBrakeWPF.Services 
{
    using System.ComponentModel.Composition.Hosting;
    using System.Linq;

    using Caliburn.Core.InversionOfControl;
    using Caliburn.MEF;
    using Caliburn.PresentationFramework.ApplicationModel;

    using HandBrakeWPF.ViewModels.Interfaces;

    public class MefBootstrapper : Bootstrapper<IMainViewModel> 
    {
        protected override IServiceLocator CreateContainer() 
        {
            var container = new CompositionContainer(new AggregateCatalog(SelectAssemblies().Select(x => new AssemblyCatalog(x))));

            return new MEFAdapter(container);
        }
    }
}