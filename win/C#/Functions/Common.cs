using System;
using System.Collections.Generic;
using System.Text;
using System.Windows.Forms;

namespace Handbrake.Functions
{
    class Common
    {
        public Boolean updateCheck()
        {
            try
            {
                Functions.RssReader rssRead = new Functions.RssReader();
                string build = rssRead.build();

                int latest = int.Parse(build);
                int current = Properties.Settings.Default.build;
                int skip = Properties.Settings.Default.skipversion;

                if (latest == skip)
                {
                    return false;
                }
                else
                {
                    Boolean update = (latest > current);
                    return update;
                }
            }
            catch (Exception exc)
            {
                MessageBox.Show(exc.ToString());
                return false;
            }
        }
    }
}
