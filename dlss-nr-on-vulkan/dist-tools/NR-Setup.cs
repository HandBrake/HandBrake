using System;
using System.IO;
using System.Management.Automation;
using System.Management.Automation.Runspaces;
using System.Reflection;
using System.Threading;
using System.Windows.Forms;

// A console-free host for the same WPF window as NR-Setup.cmd. Python is only
// needed by the backend, so the setup window can explain a missing interpreter.
internal static class NrSetup
{
    [STAThread]
    private static int Main()
    {
        try
        {
            string root = Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location);
            string source = Path.Combine(root, "windows-wizard.ps1");
            if (!File.Exists(source))
                throw new FileNotFoundException("Extract the complete DLSS-NR ZIP before starting setup.", source);
            using (Runspace runspace = RunspaceFactory.CreateRunspace())
            {
                runspace.ApartmentState = ApartmentState.STA;
                runspace.ThreadOptions = PSThreadOptions.UseCurrentThread;
                runspace.Open();
                using (PowerShell command = PowerShell.Create())
                {
                    command.Runspace = runspace;
                    command.AddScript(File.ReadAllText(source), true).AddParameter("Root", root);
                    command.Invoke();
                    if (command.HadErrors)
                        throw new InvalidOperationException(command.Streams.Error[0].ToString());
                }
            }
            return 0;
        }
        catch (Exception error)
        {
            MessageBox.Show(error.Message, "DLSS-NR setup", MessageBoxButtons.OK, MessageBoxIcon.Error);
            return 1;
        }
    }
}
