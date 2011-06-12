namespace HandBrake.Interop
{
	using System;
	using System.Collections.Generic;
	using System.Linq;
	using System.Text;

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
