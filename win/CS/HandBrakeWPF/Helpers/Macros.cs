// --------------------------------------------------------------------------------------------------------------------
// <copyright file="Macros.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The macros.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Helpers
{
    /// <summary>
    /// The macros.
    /// </summary>
    public class Macros
    {
        /// <summary>
        /// The even.
        /// #define EVEN( a )        ( (a) + ( (a) & 1 ) )
        /// </summary>
        /// <param name="a">
        /// The a.
        /// </param>
        /// <returns>
        /// The <see cref="bool"/>.
        /// </returns>
        public static bool Even(int a)
        {
            return (a) + ((a) & 1) == 1;
        }
        
        /// <summary>
        /// The multipl e_ mo d_ down.
        /// #define MULTIPLE_MOD_DOWN( a, b ) ((b==1)?a:( b * ( (a) / b ) ))
        /// </summary>
        /// <param name="a">
        /// The a.
        /// </param>
        /// <param name="b">
        /// The b.
        /// </param>
        /// <returns>
        /// The <see cref="double"/>.
        /// </returns>
        public static int MultipleModDown(int a, int b)
        {
            return (b == 1) ? a : (b * ((a) / b));
        }

        /// <summary>
        /// The multiple mod up.
        /// #define MULTIPLE_MOD_UP( a, b ) ((b==1)?a:( b * ( ( (a) + (b) - 1) / b ) ))
        /// </summary>
        /// <param name="a">
        /// The a.
        /// </param>
        /// <param name="b">
        /// The b.
        /// </param>
        /// <returns>
        /// The <see cref="double"/>.
        /// </returns>
        public static int MultipleModUp(int a, int b)
        {
            return (b == 1) ? a : (b * (((a) + (b) - 1) / b));
        }

        /// <summary>
        /// The multiple of mod x
        /// #define MULTIPLE_MOD( a, b ) ((b==1)?a:( b * ( ( (a) + (b / 2) - 1) / b ) ))
        /// </summary>
        /// <param name="a">
        /// The a.
        /// </param>
        /// <param name="b">
        /// The b.
        /// </param>
        /// <returns>
        /// The <see cref="double"/>.
        /// </returns>
        public static int MultipleMod(int a, int b)
        {
            return (b == 1) ? a : (b * (((a) + (b / 2) - 1) / b));
        }

        /// <summary>
        /// The multiple of 16.
        /// #define MULTIPLE_16( a ) ( 16 * ( ( (a) + 8 ) / 16 ) )
        /// </summary>
        /// <param name="a">
        /// The a.
        /// </param>
        /// <returns>
        /// The <see cref="bool"/>.
        /// </returns>
        public static bool Multiple16(int a)
        {
            return 16 * (((a) + 8) / 16) == 1;
        }
    }
}
