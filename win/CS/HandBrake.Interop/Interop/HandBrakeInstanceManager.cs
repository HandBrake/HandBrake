// --------------------------------------------------------------------------------------------------------------------
// <copyright file="HandBrakeInstanceManager.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The hand brake instance manager.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Interop
{
    using System;

    using HandBrake.Interop.Interop.Interfaces;
    using HandBrake.Interop.Model;

    /// <summary>
    /// The HandBrake Instance manager.
    /// Only supports scanning right now.
    /// </summary>
    public static class HandBrakeInstanceManager
    {
        private static HandBrakeInstance scanInstance;
        private static HandBrakeInstance previewInstance;
        private static HandBrakeInstance masterInstance;

        /// <summary>
        /// Initializes static members of the <see cref="HandBrakeInstanceManager"/> class.
        /// </summary>
        static HandBrakeInstanceManager()
        {
            masterInstance = new HandBrakeInstance();
            masterInstance.Initialize(2);
        }

        /// <summary>
        /// The init.
        /// </summary>
        public static void Init()
        {
            // Nothing to do. Triggers static constructor.
        }

        /// <summary>
        /// Gets the scanInstance.
        /// </summary>
        /// <param name="verbosity">
        /// The verbosity.
        /// </param>
        /// <returns>
        /// The <see cref="IHandBrakeInstance"/>.
        /// </returns>
        public static IHandBrakeInstance GetScanInstance(int verbosity)
        {
            if (scanInstance != null)
            {
                scanInstance.Dispose();
                scanInstance = null;
            }

            HandBrakeInstance newInstance = new HandBrakeInstance();
            newInstance.Initialize(verbosity);
            scanInstance = newInstance;

            return scanInstance;
        }

        /// <summary>
        /// The get encode instance.
        /// </summary>
        /// <param name="verbosity">
        /// The verbosity.
        /// </param>
        /// <param name="configuration">
        /// The configuration.
        /// </param>
        /// <returns>
        /// The <see cref="IHandBrakeInstance"/>.
        /// </returns>
        public static IHandBrakeInstance GetPreviewInstance(int verbosity, HBConfiguration configuration)
        {
            if (previewInstance != null)
            {
                previewInstance.Dispose();
                previewInstance = null;
            }

            HandBrakeInstance newInstance = new HandBrakeInstance();
            newInstance.Initialize(verbosity);
            previewInstance = newInstance;

            HandBrakeUtils.SetDvdNav(!configuration.IsDvdNavDisabled);

            return previewInstance;
        }

        /// <summary>
        /// Gets the master instance.
        /// </summary>
        internal static IHandBrakeInstance MasterInstance
        {
            get
            {
                return masterInstance;
            }
        }

        /// <summary>
        /// Gets the last scan scan instance.
        /// </summary>
        internal static IHandBrakeInstance LastScanScanInstance
        {
            get
            {
                return scanInstance;
            }
        }

        /// <summary>
        /// Gets the handle.
        /// </summary>
        internal static IntPtr LastScanHandle 
        {
            get
            {
                return scanInstance.Handle;
            }
        }
    }
}
