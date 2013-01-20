// --------------------------------------------------------------------------------------------------------------------
// <copyright file="Title.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   An object that represents a single Title of a DVD
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.SourceData
{
	using System;
	using System.Collections.Generic;

	using HandBrake.Interop.Model;

	/// <summary>
	/// An object that represents a single Title of a DVD
	/// </summary>
	public class Title
	{
	    /// <summary>
	    /// The audio tracks.
	    /// </summary>
	    private readonly List<AudioTrack> audioTracks;

	    /// <summary>
	    /// The chapters.
	    /// </summary>
	    private readonly List<Chapter> chapters;

	    /// <summary>
	    /// The subtitles.
	    /// </summary>
	    private readonly List<Subtitle> subtitles;
		
		/// <summary>
		/// Initializes a new instance of the Title class.
		/// </summary>
		public Title()
		{
			this.audioTracks = new List<AudioTrack>();
			this.chapters = new List<Chapter>();
			this.subtitles = new List<Subtitle>();
		}

		/// <summary>
		/// Gets or sets the input type of this title.
		/// </summary>
		public InputType InputType { get; set; }

		/// <summary>
		/// Gets a collection of chapters in this Title
		/// </summary>
		public List<Chapter> Chapters
		{
			get { return this.chapters; }
		}

		/// <summary>
		/// Gets a collection of audio tracks associated with this Title
		/// </summary>
		public List<AudioTrack> AudioTracks
		{
			get { return this.audioTracks; }
		}

		/// <summary>
		/// Gets a collection of subtitles associated with this Title
		/// </summary>
		public List<Subtitle> Subtitles
		{
			get { return this.subtitles; }
		}

		/// <summary>
		/// Gets or sets the track number of this Title (1-based).
		/// </summary>
		public int TitleNumber { get; set; }

		/// <summary>
		/// Gets or sets the playlist number this title came from.
		/// </summary>
		public int Playlist { get; set; }

		/// <summary>
		/// Gets or sets the length in time of this Title
		/// </summary>
		public TimeSpan Duration { get; set; }

		/// <summary>
		/// Gets or sets the resolution (width/height) of this Title
		/// </summary>
		public Size Resolution { get; set; }

		/// <summary>
		/// Gets or sets the aspect ratio of this Title
		/// </summary>
		public double AspectRatio { get; set; }

		/// <summary>
		/// Gets or sets the number of angles on the title.
		/// </summary>
		public int AngleCount { get; set; }

		/// <summary>
		/// Gets or sets the pixel aspect ratio.
		/// </summary>
		public Size ParVal { get; set; }

		/// <summary>
		/// Gets or sets the automatically detected crop region for this Title.
		/// </summary>
		public Cropping AutoCropDimensions { get; set; }

		/// <summary>
		/// Gets or sets the name of the video codec for this title.
		/// </summary>
		public string VideoCodecName { get; set; }

		/// <summary>
		/// Gets or sets the video frame rate for this title.
		/// </summary>
		public double Framerate { get; set; }

		/// <summary>
		/// Gets or sets the video frame rate numerator.
		/// </summary>
		public int FramerateNumerator { get; set; }

		/// <summary>
		/// Gets or sets the video frame rate denominator.
		/// </summary>
		public int FramerateDenominator { get; set; }

		/// <summary>
		/// Gets the total number of frames in this title.
		/// </summary>
		public int Frames
		{
			get
			{
				return (int)Math.Ceiling(((double)this.Duration.TotalSeconds) * this.Framerate);
			}
		}

		/// <summary>
		/// Override of the ToString method to provide an easy way to use this object in the UI
		/// </summary>
		/// <returns>A string representing this track in the format: {title #}[ {playlist source}] (00:00:00)</returns>
		public override string ToString()
		{
			string playlistPortion = string.Empty;
			if (this.InputType == InputType.Bluray)
			{
				playlistPortion = string.Format(" {0:d5}.MPLS", this.Playlist);
			}

			return string.Format(
				"{0}{1} ({2:00}:{3:00}:{4:00})", 
				this.TitleNumber, 
				playlistPortion,
				this.Duration.Hours,
				this.Duration.Minutes, 
				this.Duration.Seconds);
		}

		/// <summary>
		/// Gets the display string for this title.
		/// </summary>
		public string Display
		{
			get
			{
				return this.ToString();
			}
		}
	}
}