// --------------------------------------------------------------------------------------------------------------------
// <copyright file="PowerService.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The Power Service
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Services
{
    using HandBrakeWPF.Utilities;

    public class PowerService
    {
        public static bool HasBattery()
        {
            Win32.PowerState state = Win32.PowerState.GetPowerState();

            if (state == null || state.BatteryFlag == Win32.BatteryFlag.NoSystemBattery)
            {
                return false; 
            }

            return true;
        }
    }
}
