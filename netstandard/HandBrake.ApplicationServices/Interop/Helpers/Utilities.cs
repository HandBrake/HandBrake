// --------------------------------------------------------------------------------------------------------------------
// <copyright file="Utilities.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the Utilities type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ApplicationServices.Interop.Helpers
{
    /// <summary>
    /// The utilities.
    /// </summary>
    internal static class Utilities
    {
        /// <summary>
        /// Get the Greatest Common Factor
        /// </summary>
        /// <param name="a">
        /// The a.
        /// </param>
        /// <param name="b">
        /// The b.
        /// </param>
        /// <returns>
        /// The greatest common factor
        /// </returns>
        public static int GreatestCommonFactor(int a, int b)
        {
            if (a == 0)
            {
                return b;
            }

            if (b == 0)
            {
                return a;
            }

            if (a > b)
            {
                return GreatestCommonFactor(a % b, b);
            }
            
            return GreatestCommonFactor(a, b % a);
        }
    }
}
