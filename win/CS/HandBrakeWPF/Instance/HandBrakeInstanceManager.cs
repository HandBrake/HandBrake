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
    using System;
    using System.Runtime.CompilerServices;

    using HandBrake.Interop.Interop;
    using HandBrake.Interop.Interop.Interfaces;
    using HandBrake.Interop.Model;

    using HandBrakeWPF.Factories;

    /// <summary>
    /// The HandBrake Instance manager.
    /// Only supports scanning right now.
    /// </summary>
    public static class HandBrakeInstanceManager
    {
        private static IEncodeInstance encodeInstance;
        private static HandBrakeInstance scanInstance;
        private static HandBrakeInstance previewInstance;
        private static bool noHardware;

        /// <summary>
        /// The init.
        /// </summary>
        public static void Init(bool noHardwareMode)
        {
            noHardware = noHardwareMode;
            HandBrakeUtils.RegisterLogger();
            HandBrakeUtils.EnsureGlobalInit(noHardwareMode);
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
            if (!HandBrakeUtils.IsInitialised())
            {
                throw new Exception("Please call Init before Using!");
            }

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

            newInstance.Initialize(verbosity, noHardware);

            encodeInstance = newInstance;

            HandBrakeUtils.SetDvdNav(!configuration.IsDvdNavDisabled);

            return encodeInstance;
        }

        /// <summary>
        /// Gets the scanInstance.
        /// </summary>
        /// <param name="verbosity">
        /// The verbosity.
        /// </param>
        /// <param name="configuration">
        ///  HandBrakes config
        /// </param>
        /// <returns>
        /// The <see cref="IHandBrakeInstance"/>.
        /// </returns>
        public static IHandBrakeInstance GetScanInstance(int verbosity, HBConfiguration configuration)
        {
            if (!HandBrakeUtils.IsInitialised())
            {
                throw new Exception("Please call Init before Using!");
            }

            if (scanInstance != null)
            {
                scanInstance.Dispose();
                scanInstance = null;
            }

            HandBrakeInstance newInstance = new HandBrakeInstance();
            newInstance.Initialize(verbosity, noHardware);
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
            if (!HandBrakeUtils.IsInitialised())
            {
                throw new Exception("Please call Init before Using!");
            }

            if (previewInstance != null)
            {
                previewInstance.Dispose();
                previewInstance = null;
            }

            HandBrakeInstance newInstance = new HandBrakeInstance();
            newInstance.Initialize(verbosity, noHardware);
            previewInstance = newInstance;

            HandBrakeUtils.SetDvdNav(!configuration.IsDvdNavDisabled);

            return previewInstance;
        }
    }
}
