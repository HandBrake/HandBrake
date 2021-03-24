// --------------------------------------------------------------------------------------------------------------------
// <copyright file="ConsoleOutput.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the ConsoleOutput type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Worker.Logging
{
    using System;

    public class ConsoleOutput
    {
        public static void WriteLine(string text, ConsoleColor colour = ConsoleColor.White, bool enableTimecode = false)
        {
            Console.ForegroundColor = colour;
            if (enableTimecode)
            {
                string time = DateTime.Now.ToString("HH:mm:ss", System.Globalization.DateTimeFormatInfo.InvariantInfo);
                Console.WriteLine("[{0}] {1}", time, text);
            }
            else
            {
                Console.WriteLine(text);
            }
          
            Console.ResetColor();
        }

        public static void ClearLine()
        {
            int currentLineCursor = Console.CursorTop;
            Console.SetCursorPosition(0, Console.CursorTop);
            Console.Write(new string(' ', Console.WindowWidth));
            Console.SetCursorPosition(0, currentLineCursor);
        }
    }
}
