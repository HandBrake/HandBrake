/*  Chapter.cs $
	
	   This file is part of the HandBrake source code.
	   Homepage: <http://handbrake.fr>.
	   It may be used under the terms of the GNU General Public License. */

namespace HandBrake.SourceData
{
	using System;

	/// <summary>
	/// An object representing a Chapter aosciated with a Title, in a DVD
	/// </summary>
	public class Chapter
	{
		/// <summary>
		/// Gets or sets the number of this Chapter, in regards to its parent Title
		/// </summary>
		public int ChapterNumber { get; set; }

		/// <summary>
		/// Gets or sets the length in time this Chapter spans
		/// </summary>
		public TimeSpan Duration { get; set; }

		/// <summary>
		/// Override of the ToString method to make this object easier to use in the UI
		/// </summary>
		/// <returns>A string formatted as: {chapter #}</returns>
		public override string ToString()
		{
			return this.ChapterNumber.ToString();
		}
	}
}