// --------------------------------------------------------------------------------------------------------------------
// <copyright file="HandBrakeInstanceManager.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The hand brake instance manager.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ApplicationServices.Interop
{
    using System;

    using HandBrake.ApplicationServices.Interop.Interfaces;

    /// <summary>
    /// The HandBrake Instance manager.
    /// Only supports scanning right now.
    /// </summary>
    internal static class HandBrakeInstanceManager
    {
        private static HandBrakeInstance scanInstance;
        private static HandBrakeInstance encodeInstance;

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
        /// <returns>
        /// The <see cref="IHandBrakeInstance"/>.
        /// </returns>
        public static IHandBrakeInstance GetEncodeInstance(int verbosity)
        {
            if (encodeInstance != null)
            {
                encodeInstance.Dispose();
                encodeInstance = null;
            }

            HandBrakeInstance newInstance = new HandBrakeInstance();
            newInstance.Initialize(verbosity);
            encodeInstance = newInstance;

            return encodeInstance;
        }

        /// <summary>
        /// Gets the last scan scan instance.
        /// </summary>
        public static IHandBrakeInstance LastScanScanInstance
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

        /// <summary>
        /// Gets the last encode scan instance.
        /// </summary>
        public static IHandBrakeInstance LastEncodeScanInstance
        {
            get
            {
                return encodeInstance;
            }
        }

        /// <summary>
        /// Gets the encode handle.
        /// </summary>
        internal static IntPtr LastEncodeHandle
        {
            get
            {
                return encodeInstance.Handle;
            }
        }
    }
}
