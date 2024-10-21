// --------------------------------------------------------------------------------------------------------------------
// <copyright file="hb_error_code.cs" company="HandBrake Project (https://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the hb_error_code type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Interop.HbLib
{
    /// <summary>
    /// The hb_error_code.
    /// </summary>
    internal enum hb_error_code
    {
        HB_ERROR_NONE = 0,
        HB_ERROR_CANCELED,
        HB_ERROR_WRONG_INPUT,
        HB_ERROR_INIT,
        HB_ERROR_UNKNOWN,
        HB_ERROR_READ
    }
}
