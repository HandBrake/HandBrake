// --------------------------------------------------------------------------------------------------------------------
// <copyright file="Utilities.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the Utilities type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop
{
    public static class Utilities
	{
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
			else
			{
				return GreatestCommonFactor(a, b % a);
			}
		}
	}
}
