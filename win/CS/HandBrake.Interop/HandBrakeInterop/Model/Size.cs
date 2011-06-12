namespace HandBrake.Interop
{
	public class Size
	{
		public Size(int width, int height)
		{
			this.Width = width;
			this.Height = height;
		}

		public int Width { get; set; }
		public int Height { get; set; }
	}
}
