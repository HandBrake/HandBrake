/*  AudioTrack.cs $
	
	   This file is part of the HandBrake source code.
	   Homepage: <http://handbrake.fr>.
	   It may be used under the terms of the GNU General Public License. */

namespace HandBrake.SourceData
{
	/// <summary>
	/// An object represending an AudioTrack associated with a Title, in a DVD
	/// </summary>
	public class AudioTrack
	{
		/// <summary>
		/// Gets or sets the track number of this Audio Track
		/// </summary>
		public int TrackNumber { get; set; }

		/// <summary>
		/// Gets or sets the audio codec of this Track.
		/// </summary>
		public AudioCodec Codec { get; set; }

		/// <summary>
		/// Gets or sets the language (if detected) of this Audio Track
		/// </summary>
		public string Language { get; set; }

		/// <summary>
		/// Gets or sets the language code for this audio track.
		/// </summary>
		public string LanguageCode { get; set; }

		/// <summary>
		/// Gets or sets the description for this audio track.
		/// </summary>
		public string Description { get; set; }

		/// <summary>
		/// Gets or sets the channel layout of this Audio Track.
		/// </summary>
		public int ChannelLayout { get; set; }

		/// <summary>
		/// Gets or sets the frequency (in Hz) of this Audio Track
		/// </summary>
		public int SampleRate { get; set; }

		/// <summary>
		/// Gets or sets the bitrate (in bits/sec) of this Audio Track.
		/// </summary>
		public int Bitrate { get; set; }

		/// <summary>
		/// Gets the display string for this audio track.
		/// </summary>
		public string Display
		{
			get
			{
				return this.GetDisplayString(true);
			}
		}

		/// <summary>
		/// Gets the display string for this audio track (not including track number)
		/// </summary>
		public string NoTrackDisplay
		{
			get
			{
				return this.GetDisplayString(false);
			}
		}

		/// <summary>
		/// Override of the ToString method to make this object easier to use in the UI
		/// </summary>
		/// <returns>A string formatted as: {track #} {language} ({format}) ({sub-format})</returns>
		public override string ToString()
		{
			return this.GetDisplayString(true);
		}

		private string GetDisplayString(bool includeTrackNumber)
		{
			if (includeTrackNumber)
			{
				return this.TrackNumber + " " + this.Description;
			}
			else
			{
				return this.Description;
			}
		}
	}
}