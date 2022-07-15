// --------------------------------------------------------------------------------------------------------------------
// <copyright file="IWindowManager.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Services.Interfaces
{
    using System.Collections.Generic;
    using System.Threading.Tasks;

    public interface IWindowManager
    {
        Task<bool?> ShowDialogAsync(
            object rootModel,
            object context = null,
            IDictionary<string, object> settings = null);

        Task ShowWindowAsync(
            object rootModel,
            object context = null,
            IDictionary<string, object> settings = null);

        Task ShowPopupAsync(
            object rootModel,
            object context = null,
            IDictionary<string, object> settings = null);
    }
}
