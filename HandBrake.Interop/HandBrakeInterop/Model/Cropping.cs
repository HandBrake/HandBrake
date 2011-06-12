namespace HandBrake.Interop
{
	public class Cropping
	{
		public int Top { get; set; }
		public int Bottom { get; set; }
		public int Left { get; set; }
		public int Right { get; set; }

		public Cropping Clone()
		{
			return new Cropping
			{
				Top = this.Top,
				Bottom = this.Bottom,
				Left = this.Left,
				Right = this.Right
			};
		}
	}
}
