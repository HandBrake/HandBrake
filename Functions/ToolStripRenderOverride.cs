/*  ToolStripRenderOverride.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr/>.
    It may be used under the terms of the GNU General Public License. */

namespace Handbrake.Functions
{
    using System.Windows.Forms;

    public class ToolStripRenderOverride : ToolStripProfessionalRenderer
    {
        ToolStripSystemRenderer render = new ToolStripSystemRenderer();
    
        public ToolStripRenderOverride()
        {       
        }

        /// <summary>
        /// Fix for a bug in .NET where a white line appears at the bottom of the control.
        /// http://connect.microsoft.com/VisualStudio/feedback/details/92862/toolstrip-always-draws-a-border-on-the-bottom-with-rendermode-system-and-docked-left-or-right
        /// </summary>
        /// <param name="e">
        /// ToolStripRenderEventArgs 
        /// </param>
        protected override void OnRenderToolStripBorder(ToolStripRenderEventArgs e)
        {
            //base.OnRenderToolStripBorder(e);
        }

        /// <summary>
        /// Change the Button Hover Background style to the system button style
        /// </summary>
        /// <param name="e">
        /// ToolStripRenderEventArgs
        /// </param>
        protected override void OnRenderButtonBackground(ToolStripItemRenderEventArgs e)
        {     
            render.DrawButtonBackground(e);
        }

        protected override void OnRenderDropDownButtonBackground(ToolStripItemRenderEventArgs e)
        {
            render.DrawDropDownButtonBackground(e);
        }

        protected override void OnRenderToolStripBackground(ToolStripRenderEventArgs e)
        {
            //base.OnRenderToolStripBackground(e);
        }  
    }
}
