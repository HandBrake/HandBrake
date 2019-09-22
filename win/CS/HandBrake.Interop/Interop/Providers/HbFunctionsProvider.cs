// --------------------------------------------------------------------------------------------------------------------
// <copyright file="HbFunctionsProvider.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Provides an instance of IHbFunctions for use by the GUI.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Interop.Providers
{
    using HandBrake.Interop.Interop.HbLib.Wrappers;
    using HandBrake.Interop.Interop.HbLib.Wrappers.Interfaces;
    using HandBrake.Interop.Interop.Providers.Interfaces;

    public class HbFunctionsProvider : IHbFunctionsProvider
    {
        private static IHbFunctions hbFunctions;

        public IHbFunctions GetHbFunctionsWrapper()
        {
            if (hbFunctions == null)
            {
                hbFunctions = new HbFunctionsDirect();
            }

            return hbFunctions;
        }
    }
}
