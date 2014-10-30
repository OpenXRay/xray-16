using System;
using System.Collections.Generic;
using System.Linq;
using System.Windows.Forms;
using XRay.ManagedApi.Core;

namespace xrPostprocessEditor
{
    static class Program
    {
        /// <summary>
        /// The main entry point for the application.
        /// </summary>
        [STAThread]
        static void Main()
        {
            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);
            Core.Initialize(Application.ProductName);
            Application.Run(new MainDialog());
            Core.Destroy();
        }
    }
}
