using System.Runtime.InteropServices;

namespace Handbrake.Functions
{
    class Win32
    {
        [DllImport("user32.dll")]
        public static extern int SetForegroundWindow(int hWnd);
    }
}
