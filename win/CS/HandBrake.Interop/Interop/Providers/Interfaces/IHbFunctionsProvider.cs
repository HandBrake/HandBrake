// --------------------------------------------------------------------------------------------------------------------
// <copyright file="IHbFunctionsProvider.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Provides an instance of IHbFunctions for use by the GUI.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Interop.Providers.Interfaces
{
    using HandBrake.Interop.Interop.HbLib.Wrappers.Interfaces;

    public interface IHbFunctionsProvider
    {
        IHbFunctions GetHbFunctionsWrapper();
    }
}