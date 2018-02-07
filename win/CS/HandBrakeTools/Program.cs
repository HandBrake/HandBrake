// --------------------------------------------------------------------------------------------------------------------
// <copyright file="Program.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   HandBrake Toolkit
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeTools
{
    using System;
    using System.Diagnostics;
    using System.IO;
    using System.Security.Cryptography;

    public class Program
    {
        static void Main(string[] args)
        {
            if (args.Length == 0)
            {
                Console.WriteLine("Invalid Command.  Either 'genkeys' or 'sign <filename>'");
                Console.Read();
                return;
            }

            string command = args[0];
            string file = args.Length > 1 ? args[1] : null;
            switch (command)
            {
                case "genkeys":
                    GenKeyFiles();
                    return;
                case "sign":
                    string hash = SignDownload(file);
                    Console.WriteLine(VerifyDownload(hash, file) ? "Passed Verification Test" : "Failed Verification Test");
                    Console.WriteLine("Hash: " + hash);
                    File.WriteAllText("file.hash", hash);
                    return;
            }

            Console.WriteLine("Done.");
            Console.Read();
        }

        public static void GenKeyFiles()
        {
            RSACryptoServiceProvider provider = new RSACryptoServiceProvider { KeySize = 2048 };

            using (StreamWriter sw = new StreamWriter("private.key"))
            {
                sw.Write(provider.ToXmlString(true));
            }

            using (StreamWriter sw = new StreamWriter("public.key"))
            {
                sw.Write(provider.ToXmlString(false));
            }
        }

        public static string SignDownload(string updateFile)
        {
            RSACryptoServiceProvider provider = new RSACryptoServiceProvider();
            provider.FromXmlString(File.ReadAllText("private.key"));
            byte[] signedBytes = provider.SignData(File.ReadAllBytes(updateFile), "SHA256");
            return Convert.ToBase64String(signedBytes);
        }

        public static bool VerifyDownload(string signature, string updateFile)
        {
            // Sanity Checks
            if (!File.Exists(updateFile))
            {
                return false;
            }

            if (string.IsNullOrEmpty(signature))
            {
                return false;
            }

            // Fetch our Public Key
            string publicKey = File.ReadAllText("public.key");

            // Verify the file against the Signature. 
            try
            {
                byte[] file = File.ReadAllBytes(updateFile);
                RSACryptoServiceProvider verifyProfider = new RSACryptoServiceProvider();
                verifyProfider.FromXmlString(publicKey);
                return verifyProfider.VerifyData(file, "SHA256", Convert.FromBase64String(signature));
            }
            catch (Exception e)
            {
                Debug.WriteLine(e);
                return false;
            }
        }
    }
}
