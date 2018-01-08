namespace HandBrakeUWP
{
    using System;
    using System.Collections.Generic;
    using Caliburn.Micro;
    using PlatformBindings;
    using Windows.ApplicationModel.Activation;
    using Windows.UI.Xaml.Controls;

    /// <summary>
    /// Provides application-specific behavior to supplement the default Application class.
    /// </summary>
    public sealed partial class App : CaliburnApplication
    {
        private WinRTContainer container;

        /// <summary>
        /// Initializes a new instance of the <see cref="App"/> class.
        /// </summary>
        public App()
        {
            PlatformBindingsBootstrapper.Initialise(true);
            new UWPHandBrakeServices();
            this.InitializeComponent();
        }

        /// <summary>
        /// Configures Viewmodels, Services, etc.
        /// </summary>
        protected override void Configure()
        {
            this.container = new WinRTContainer();
            this.container.RegisterWinRTServices();

            //TODO: Register your view models at the container
        }

        protected override object GetInstance(Type service, string key)
        {
            var instance = this.container.GetInstance(service, key);
            if (instance != null)
            {
                return instance;
            }

            throw new Exception("Could not locate any instances.");
        }

        protected override IEnumerable<object> GetAllInstances(Type service)
        {
            return this.container.GetAllInstances(service);
        }

        protected override void BuildUp(object instance)
        {
            this.container.BuildUp(instance);
        }

        protected override void PrepareViewFirst(Frame rootFrame)
        {
            this.container.RegisterNavigationService(rootFrame);
        }

        protected override void OnLaunched(LaunchActivatedEventArgs args)
        {
            if (args.PreviousExecutionState == ApplicationExecutionState.Running)
            {
                return;
            }

            this.DisplayRootView<MainPage>();
        }
    }
}