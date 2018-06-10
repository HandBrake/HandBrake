// --------------------------------------------------------------------------------------------------------------------
// <copyright file="HandBrakeInstanceManager.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The hand brake instance manager.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Instance
{
    using HandBrake.Interop.Interop;
    using HandBrake.Interop.Interop.Interfaces;
    using HandBrake.Interop.Model;

    /// <summary>
    /// The HandBrake Instance manager.
    /// Only supports scanning right now.
    /// </summary>
    public static class HandBrakeInstanceManager
    {
        private static IEncodeInstance encodeInstance;

        /// <summary>
        /// Initializes static members of the <see cref="HandBrakeInstanceManager"/> class.
        /// </summary>
        static HandBrakeInstanceManager()
        {
        }

        /// <summary>
        /// The init.
        /// </summary>
        public static void Init()
        {
            // Nothing to do. Triggers static constructor.
        }

        /// <summary>
        /// The get encode instance.
        /// </summary>
        /// <param name="verbosity">
        /// The verbosity.
        /// </param>
        /// <param name="configuration">
        /// The configuratio.
        /// </param>
        /// <returns>
        /// The <see cref="IHandBrakeInstance"/>.
        /// </returns>
        public static IEncodeInstance GetEncodeInstance(int verbosity, HBConfiguration configuration)
        {
            if (encodeInstance != null)
            {
                encodeInstance.Dispose();
                encodeInstance = null;
            }

            IEncodeInstance newInstance;

            if (configuration.RemoteServiceEnabled)
            {
                newInstance = new RemoteInstance(configuration.RemoteServicePort);
            }
            else
            {
                newInstance = new HandBrakeInstance();
            }
                
            newInstance.Initialize(verbosity);
            encodeInstance = newInstance;

            HandBrakeUtils.SetDvdNav(!configuration.IsDvdNavDisabled);

            return encodeInstance;
        }
    }
}
