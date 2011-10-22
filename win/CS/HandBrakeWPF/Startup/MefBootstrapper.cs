// --------------------------------------------------------------------------------------------------------------------
// <copyright file="MefBootstrapper.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The MEF Bootstrapper (Not Used)
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Startup 
{
    using System;
    using System.Collections.Generic;
    using System.ComponentModel.Composition;
    using System.ComponentModel.Composition.Hosting;
    using System.Linq;

    using Caliburn.Micro;

    using HandBrakeWPF.ViewModels.Interfaces;

    /// <summary>
    /// The MEF Bootstrapper (Not Used)
    /// </summary>
    public class MefBootstrapper : Bootstrapper<IMainViewModel>
    {
        /// <summary>
        /// The Backing field for the container
        /// </summary>
        private CompositionContainer container;

        /// <summary>
        /// MEF Configure
        /// </summary>
        protected override void Configure()
        {
            container = new CompositionContainer(
                new AggregateCatalog(AssemblySource.Instance.Select(x => new AssemblyCatalog(x)))
                );

            var batch = new CompositionBatch();

            batch.AddExportedValue<IWindowManager>(new WindowManager());
            batch.AddExportedValue<IEventAggregator>(new EventAggregator());
            batch.AddExportedValue(container);

            container.Compose(batch);
        }

        /// <summary>
        /// Get an Instance of a service
        /// </summary>
        /// <param name="serviceType">
        /// The service.
        /// </param>
        /// <param name="key">
        /// The key.
        /// </param>
        /// <returns>
        /// The Service Requested
        /// </returns>
        protected override object GetInstance(Type serviceType, string key)
        {
            string contract = string.IsNullOrEmpty(key) ? AttributedModelServices.GetContractName(serviceType) : key;
            var exports = container.GetExportedValues<object>(contract);

            if (exports.Count() > 0)
                return exports.First();

            throw new Exception(string.Format("Could not locate any instances of contract {0}.", contract));
        }

        /// <summary>
        /// Get all instances of a service
        /// </summary>
        /// <param name="serviceType">
        /// The service.
        /// </param>
        /// <returns>
        /// A collection of instances of the requested service type.
        /// </returns>
        protected override IEnumerable<object> GetAllInstances(Type serviceType)
        {
            return container.GetExportedValues<object>(AttributedModelServices.GetContractName(serviceType));
        }

        /// <summary>
        /// Build Up
        /// </summary>
        /// <param name="instance">
        /// The instance.
        /// </param>
        protected override void BuildUp(object instance)
        {
            container.SatisfyImportsOnce(instance);
        }
    }
}