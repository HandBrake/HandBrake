// --------------------------------------------------------------------------------------------------------------------
// <copyright file="IWindowManager.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Services.Interfaces
{
    using System.Windows;
    
    public interface IWindowManager
    {
        bool? ShowDialog<W>(object viewModel) where W : Window, new();

        void ShowWindow<W>(object viewModelInstance) where W : Window, new();
    }
}
